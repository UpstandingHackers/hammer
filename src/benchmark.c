#include <stdio.h>
#include <time.h>
#include <string.h>
#include "hammer.h"
#include "internal.h"

#ifdef __MACH__
#include <mach/clock.h>
#include <mach/mach.h>
#endif

void h_benchmark_clock_gettime(struct timespec *ts) {
#ifdef __MACH__ // OS X does not have clock_gettime, use clock_get_time
  /* 
   * This returns real time, not CPU time. See http://stackoverflow.com/a/6725161
   * Possible solution: http://stackoverflow.com/a/11659289
   */
  clock_serv_t cclock;
  mach_timespec_t mts;
  host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
  clock_get_time(cclock, &mts);
  mach_port_deallocate(mach_task_self(), cclock);
  ts->tv_sec = mts.tv_sec;
  ts->tv_nsec = mts.tv_nsec;
#else
  clock_gettime(CLOCK_THREAD_CPUTIME_ID, ts);
#endif
}

/*
  Usage:
  Create your parser (i.e., const HParser*), and an array of test cases
  (i.e., HParserTestcase[], terminated by { NULL, 0, NULL }) and then call

  HBenchmarkResults* results = h_benchmark(parser, testcases);

  Then, you can format a report with:

  h_benchmark_report(stdout, results);

  or just generate code to make the parser run as fast as possible with:

  h_benchmark_dump_optimized_code(stdout, results);

*/

HBenchmarkResults *h_benchmark(HParser* parser, HParserTestcase* testcases) {
  return h_benchmark__m(&system_allocator, parser, testcases);
}

HBenchmarkResults *h_benchmark__m(HAllocator* mm__, HParser* parser, HParserTestcase* testcases) {
  // For now, just output the results to stderr
  HParserTestcase* tc = testcases;
  HParserBackend backend = PB_MIN;
  HBenchmarkResults *ret = h_new(HBenchmarkResults, 1);
  ret->len = PB_MAX-PB_MIN+1;
  ret->results = h_new(HBackendResults, ret->len);

  for (backend = PB_MIN; backend <= PB_MAX; backend++) {
    ret->results[backend].backend = backend;
    // Step 1: Compile grammar for given parser...
    if (h_compile(parser, backend, NULL) == -1) {
      // backend inappropriate for grammar...
      fprintf(stderr, "failed\n");
      ret->results[backend].compile_success = false;
      ret->results[backend].n_testcases = 0;
      ret->results[backend].failed_testcases = 0;
      ret->results[backend].cases = NULL;
      continue;
    }
    ret->results[backend].compile_success = true;
    int tc_failed = 0;
    // Step 1: verify all test cases.
    ret->results[backend].n_testcases = 0;
    ret->results[backend].failed_testcases = 0;
    for (tc = testcases; tc->input != NULL; tc++) {
      ret->results[backend].n_testcases++;
      HParseResult *res = h_parse(parser, tc->input, tc->length);
      char* res_unamb;
      if (res != NULL) {
	res_unamb = h_write_result_unamb(res->ast);
      } else
	res_unamb = NULL;
      if ((res_unamb == NULL && tc->output_unambiguous != NULL)
	  || (res_unamb != NULL && strcmp(res_unamb, tc->output_unambiguous) != 0)) {
	// test case failed...
	fprintf(stderr, "failed\n");
	// We want to run all testcases, for purposes of generating a
	// report. (eg, if users are trying to fix a grammar for a
	// faster backend)
	tc_failed++;
	ret->results[backend].failed_testcases++;
      }
      h_parse_result_free(res);
    }

    if (tc_failed > 0) {
      // Can't use this parser; skip to the next
      fprintf(stderr, "Backend failed testcases; skipping benchmark\n");
      continue;
    }

    ret->results[backend].cases = h_new(HCaseResult, ret->results[backend].n_testcases);
    size_t cur_case = 0;

    for (tc = testcases; tc->input != NULL; tc++) {
      // The goal is to run each testcase for at least 50ms each
      // TODO: replace this with a posix timer-based benchmark. (cf. timerfd_create, timer_create, setitimer)
      int count = 1, cur;
      struct timespec ts_start, ts_end;
      long long time_diff;
      do {
	count *= 2; // Yes, this means that the first run will run the function twice. This is fine, as we want multiple runs anyway.
  h_benchmark_clock_gettime(&ts_start);
	for (cur = 0; cur < count; cur++) {
	  h_parse_result_free(h_parse(parser, tc->input, tc->length));
	}
  h_benchmark_clock_gettime(&ts_end);

	// time_diff is in ns
	time_diff = (ts_end.tv_sec - ts_start.tv_sec) * 1000000000 + (ts_end.tv_nsec - ts_start.tv_nsec);
      } while (time_diff < 100000000);
      ret->results[backend].cases[cur_case].parse_time = (time_diff / count);
      cur_case++;
    }
  }
  return ret;
}

void h_benchmark_report(FILE* stream, HBenchmarkResults* result) {
  for (size_t i=0; i<result->len; ++i) {
    fprintf(stream, "Backend %ld ... \n", i);
    for (size_t j=0; j<result->results[i].n_testcases; ++j) {
      if(result->results[i].cases == NULL)
        continue;
      fprintf(stream, "Case %ld: %ld ns/parse\n", j,  result->results[i].cases[j].parse_time);
    }
  }
}
