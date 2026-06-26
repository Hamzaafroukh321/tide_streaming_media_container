#include "test_fixture.h"

#include "tide/repair.h"

static int repair_drops_incomplete_group(void) {
  tide_source *source = NULL;
  tide_repair_plan *plan = NULL;
  tide_source *repaired = NULL;
  size_t truncated_size;
  TIDE_EXPECT(tide_write_integration_sample("repair_in.tide") == 0);
  TIDE_EXPECT_STATUS(tide_source_from_file(&source, "repair_in.tide"), TIDE_STATUS_OK);
  truncated_size = tide_source_size(source) - 3u;
  tide_source_destroy(source);
  source = NULL;
  {
    FILE *in = fopen("repair_in.tide", "rb");
    FILE *out = fopen("repair_truncated.tide", "wb");
    size_t i;
    TIDE_EXPECT(in != NULL);
    TIDE_EXPECT(out != NULL);
    for (i = 0; i < truncated_size; ++i) {
      int ch = fgetc(in);
      TIDE_EXPECT(ch != EOF);
      TIDE_EXPECT(fputc(ch, out) != EOF);
    }
    fclose(in);
    fclose(out);
  }
  TIDE_EXPECT_STATUS(tide_source_from_file(&source, "repair_truncated.tide"), TIDE_STATUS_OK);
  TIDE_EXPECT_STATUS(tide_repair_scan(source, NULL, &plan), TIDE_STATUS_PARTIAL);
  TIDE_EXPECT(tide_repair_plan_valid_prefix(plan) > TIDE_HEADER_SIZE);
  TIDE_EXPECT_STATUS(tide_repair_write(plan, "repair_out.tide"), TIDE_STATUS_OK);
  TIDE_EXPECT_STATUS(tide_source_from_file(&repaired, "repair_out.tide"), TIDE_STATUS_OK);
  TIDE_EXPECT(tide_source_size(repaired) == (size_t)tide_repair_plan_valid_prefix(plan));
  tide_source_destroy(repaired);
  tide_repair_plan_destroy(plan);
  tide_source_destroy(source);
  remove("repair_in.tide");
  remove("repair_truncated.tide");
  remove("repair_out.tide");
  return 0;
}

int tide_repair_tests(tide_test_case *out, int max) {
  int n = 0;
  if (max > n) out[n++] = (tide_test_case){"RepairDropsIncompleteGroup", repair_drops_incomplete_group};
  return n;
}
