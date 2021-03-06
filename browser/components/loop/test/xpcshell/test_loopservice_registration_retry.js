/* Any copyright is dedicated to the Public Domain.
   http://creativecommons.org/publicdomain/zero/1.0/ */

Cu.import("resource://gre/modules/Task.jsm");
Cu.import("resource://services-common/utils.js");

/**
 * Tests that it's possible to retry registration after an error.
 */

add_test(function test_retry_after_failed_push_reg() {
  mockPushHandler.registrationResult = "404";

  MozLoopService.initialize().then((result) => {
    do_print(result);
    do_throw("should not succeed when loop server registration fails");
  }, Task.async(function*(err) {
    // 404 is an expected failure indicated by the lack of route being set
    // up on the Loop server mock. This is added in the next test.
    Assert.equal(err.message, "404", "");

    let regError = MozLoopService.errors.get("initialization");
    Assert.ok(regError, "registration error should be reported");
    Assert.ok(regError.friendlyDetailsButtonCallback, "registration retry callback should exist");

    // Remove the error
    mockPushHandler.registrationResult = null;
    mockPushHandler.registrationPushURL = kEndPointUrl;

    yield regError.friendlyDetailsButtonCallback();
    Assert.strictEqual(MozLoopService.errors.size, 0, "Check that the errors are gone");
    let deferredRegistrations = MozLoopServiceInternal.deferredRegistrations;
    yield deferredRegistrations.get(LOOP_SESSION_TYPE.GUEST).promise.then(() => {
      Assert.ok(true, "The retry of registration succeeded");
    },
    (error) => {
      Assert.ok(false, "The retry of registration should have succeeded");
    });

    run_next_test();
  }));
});

function run_test() {
  setupFakeLoopServer();

  loopServer.registerPathHandler("/registration", (request, response) => {
    let body = CommonUtils.readBytesFromInputStream(request.bodyInputStream);
    let data = JSON.parse(body);
    Assert.equal(data.simplePushURLs.calls, kEndPointUrl,
                 "Should send correct calls push url");
    Assert.equal(data.simplePushURLs.rooms, kEndPointUrl,
                 "Should send correct rooms push url");

    response.setStatusLine(null, 200, "OK");
  });

  let nowSeconds = Date.now() / 1000;
  Services.prefs.setIntPref("loop.urlsExpiryTimeSeconds", nowSeconds + 60);

  do_register_cleanup(function() {
    Services.prefs.clearUserPref("loop.hawk-session-token");
    Services.prefs.clearUserPref("loop.urlsExpiryTimeSeconds");
  });

  run_next_test();
}
