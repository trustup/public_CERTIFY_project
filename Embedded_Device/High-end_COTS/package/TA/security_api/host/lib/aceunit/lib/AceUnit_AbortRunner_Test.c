#include <assert.h>
#include <stdlib.h>
#include <aceunit.h>

bool ran_tc_ok = false;

void tc_failing(void) {
    assert(false);
}

void tc_ok(void) {
    ran_tc_ok = true;
}

static const aceunit_func_t failing_test[] = {
    tc_failing,
    tc_ok,
    NULL,
};

const AceUnit_Fixture_t failing_test_fixture = { NULL, NULL, NULL, NULL, failing_test };

const AceUnit_Fixture_t *fixtures[] = {
    &failing_test_fixture,
    NULL,
};

int main(void) {
#if defined(__BCC__)
    AceUnit_Result_t result;
    result.testCaseCount = 0;
    result.successCount = 0;
    result.failureCount = 0;
#else
    AceUnit_Result_t result = { 0, 0, 0 };
#endif
    AceUnit_run(fixtures, &result);
#include <assert.h>
    assert(result.testCaseCount == 2);
    assert(result.successCount == 1);
    assert(result.failureCount == 1);
    return EXIT_SUCCESS;
}
