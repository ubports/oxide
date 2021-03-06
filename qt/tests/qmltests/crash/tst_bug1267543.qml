import QtQuick 2.0
import QtTest 1.0
import com.canonical.Oxide 1.0
import Oxide.testsupport 1.0

TestWebView {
  id: webView
  focus: true

  TestCase {
    name: "bug1267543"
    when: windowShown

    function test_bug1267543() {
      webView.url = "http://testsuite/geolocation.html";
      verify(webView.waitForLoadSucceeded(),
             "Timed out waiting for successful load");
    }
  }
}
