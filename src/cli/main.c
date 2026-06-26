#include "tide/demux.h"
#include "tide/mux.h"
#include "tide/repair.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int status_to_exit(tide_status status) {
  switch (status) {
    case TIDE_STATUS_OK: return 0;
    case TIDE_STATUS_INVALID_ARGUMENT: return 2;
    case TIDE_STATUS_FORMAT:
    case TIDE_STATUS_UNSUPPORTED:
    case TIDE_STATUS_INTEGRITY:
    case TIDE_STATUS_TIMELINE:
    case TIDE_STATUS_INDEX: return 3;
    case TIDE_STATUS_RESOURCE: return 4;
    case TIDE_STATUS_CANCELLED: return 5;
    case TIDE_STATUS_PARTIAL: return 6;
    default: return 10;
  }
}

static int inspect_cmd(const char *path) {
  tide_source *source = NULL;
  tide_demux *demux = NULL;
  tide_demux_options options;
  tide_status status;
  size_t i;
  memset(&options, 0, sizeof(options));
  options.size = sizeof(options);
  options.limits = tide_limits_default();
  status = tide_source_from_file(&source, path);
  if (status != TIDE_STATUS_OK) {
    fprintf(stderr, "source: %s\n", tide_status_string(status));
    return status_to_exit(status);
  }
  status = tide_demux_open(&demux, source, &options);
  if (status != TIDE_STATUS_OK && status != TIDE_STATUS_PARTIAL) {
    fprintf(stderr, "decode: %s\n", tide_status_string(status));
    tide_source_destroy(source);
    return status_to_exit(status);
  }
  printf("status=%s\n", tide_status_string(status));
  printf("streams=%zu\n", tide_demux_stream_count(demux));
  for (i = 0; i < tide_demux_stream_count(demux); ++i) {
    tide_stream_info stream;
    if (tide_demux_stream_info(demux, i, &stream) == TIDE_STATUS_OK) {
      printf("stream track=%u generation=%u time_base=%u/%u codec=%u\n",
             stream.track_id,
             stream.generation,
             stream.time_base.numerator,
             stream.time_base.denominator,
             stream.codec_tag);
    }
  }
  tide_demux_close(demux);
  tide_source_destroy(source);
  return status_to_exit(status);
}

static int remux_cmd(const char *in_path, const char *out_path) {
  tide_source *source = NULL;
  tide_demux *demux = NULL;
  tide_mux *mux = NULL;
  tide_demux_options demux_options;
  tide_mux_options mux_options;
  tide_status status;
  size_t i;
  memset(&demux_options, 0, sizeof(demux_options));
  demux_options.size = sizeof(demux_options);
  demux_options.limits = tide_limits_default();
  memset(&mux_options, 0, sizeof(mux_options));
  mux_options.size = sizeof(mux_options);
  mux_options.limits = tide_limits_default();
  mux_options.movie_time_base = (tide_rational){1u, 1000u};
  mux_options.write_footer = 1;
  status = tide_source_from_file(&source, in_path);
  if (status == TIDE_STATUS_OK) {
    status = tide_demux_open(&demux, source, &demux_options);
  }
  if (status == TIDE_STATUS_OK) {
    status = tide_mux_create(&mux, out_path, &mux_options);
  }
  if (status == TIDE_STATUS_OK) {
    for (i = 0; i < tide_demux_stream_count(demux); ++i) {
      tide_stream_info stream;
      status = tide_demux_stream_info(demux, i, &stream);
      if (status == TIDE_STATUS_OK) {
        status = tide_mux_add_stream(mux, &stream);
      }
      if (status != TIDE_STATUS_OK) {
        break;
      }
    }
  }
  while (status == TIDE_STATUS_OK) {
    tide_packet_ref packet;
    tide_packet_ref_init(&packet);
    status = tide_demux_next(demux, &packet);
    if (status == TIDE_STATUS_WOULD_BLOCK) {
      status = TIDE_STATUS_OK;
      break;
    }
    if (status == TIDE_STATUS_OK) {
      status = tide_mux_write_packet(mux, &packet);
    }
    tide_packet_ref_reset(&packet);
  }
  if (mux != NULL) {
    tide_status close_status = tide_mux_close(mux);
    if (status == TIDE_STATUS_OK) {
      status = close_status;
    }
  }
  tide_demux_close(demux);
  tide_source_destroy(source);
  if (status != TIDE_STATUS_OK) {
    fprintf(stderr, "remux: %s\n", tide_status_string(status));
  }
  return status_to_exit(status);
}

static int repair_cmd(const char *in_path, const char *out_path) {
  tide_source *source = NULL;
  tide_repair_plan *plan = NULL;
  tide_status status = tide_source_from_file(&source, in_path);
  if (status == TIDE_STATUS_OK) {
    status = tide_repair_scan(source, NULL, &plan);
  }
  if (status == TIDE_STATUS_OK || status == TIDE_STATUS_PARTIAL) {
    printf("valid_prefix=%llu\n", (unsigned long long)tide_repair_plan_valid_prefix(plan));
    status = tide_repair_write(plan, out_path);
  }
  tide_repair_plan_destroy(plan);
  tide_source_destroy(source);
  if (status != TIDE_STATUS_OK) {
    fprintf(stderr, "repair: %s\n", tide_status_string(status));
  }
  return status_to_exit(status);
}

static void usage(void) {
  fprintf(stderr, "usage:\n");
  fprintf(stderr, "  tide inspect <file>\n");
  fprintf(stderr, "  tide demux <file>\n");
  fprintf(stderr, "  tide remux <in> <out>\n");
  fprintf(stderr, "  tide index <file>\n");
  fprintf(stderr, "  tide repair <in> <out>\n");
}

int main(int argc, char **argv) {
  if (argc < 3) {
    usage();
    return 2;
  }
  if (strcmp(argv[1], "inspect") == 0 && argc == 3) {
    return inspect_cmd(argv[2]);
  }
  if (strcmp(argv[1], "demux") == 0 && argc == 3) {
    return inspect_cmd(argv[2]);
  }
  if (strcmp(argv[1], "index") == 0 && argc == 3) {
    return inspect_cmd(argv[2]);
  }
  if (strcmp(argv[1], "remux") == 0 && argc == 4) {
    return remux_cmd(argv[2], argv[3]);
  }
  if (strcmp(argv[1], "repair") == 0 && argc == 4) {
    return repair_cmd(argv[2], argv[3]);
  }
  usage();
  return 2;
}
