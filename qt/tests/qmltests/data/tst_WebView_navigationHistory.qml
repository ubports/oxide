import QtQuick 2.0
import QtTest 1.0
import com.canonical.Oxide 0.1
import com.canonical.Oxide.Testing 0.1

TestWebView {
  id: webView
  focus: true
  width: 200
  height: 200

  ListView {
    id: navigationView
    model: webView.navigationHistory
    currentIndex: model.currentIndex
    readonly property url currentUrl: currentItem ? currentItem.url : ""
    readonly property string currentTitle: currentItem ? currentItem.title : ""
    delegate: Item {
      readonly property string url: model.url
      readonly property string title: model.title
    }
  }

  TestCase {
    id: test
    name: "WebView_navigationHistory"
    when: windowShown

    readonly property alias count: navigationView.count
    readonly property alias currentIndex: navigationView.currentIndex
    readonly property alias currentUrl: navigationView.currentUrl
    readonly property alias currentTitle: navigationView.currentTitle

    function verifyLoadSucceeded() {
      verify(webView.waitForLoadSucceeded(),
             "Timed out waiting for a successful load");
    }

    function loadUrl(url) {
      webView.url = url;
      verifyLoadSucceeded();
    }

    function compareAttributes(rCount, rCurrentIndex, rCurrentUrl,
                               rCurrentTitle, message) {
      compare(count, rCount, message);
      compare(currentIndex, rCurrentIndex, message);
      compare(currentUrl, rCurrentUrl, message);
      compare(currentTitle, rCurrentTitle, message);
    }

    function test_WebView_navigationHistory() {
      compareAttributes(0, -1, "", "",
                        "Navigation history should be initially empty");

      var url1 = "http://localhost:8080/tst_WebView_navigation1.html";
      var title1 = "Navigation test 1";
      loadUrl(url1);
      compareAttributes(1, 0, url1, title1,
                        "One entry / current is the first one");

      var url2 = "http://localhost:8080/tst_WebView_navigation2.html";
      var title2 = "Navigation test 2";
      loadUrl(url2);
      compareAttributes(2, 1, url2, title2,
                        "Two entries / current entry is the last one");

      webView.goBack();
      verifyLoadSucceeded();
      compareAttributes(2, 0, url1, title1,
                        "No new entries / current is the first one");

      webView.goForward();
      verifyLoadSucceeded();
      compareAttributes(2, 1, url2, title2,
                        "No new entries / current is the last one");

      webView.navigationHistory.currentIndex = 0;
      verifyLoadSucceeded();
      compareAttributes(2, 0, url1, title1,
                        "No new entries / current is the first one");

      var url3 = "http://localhost:8080/tst_WebView_navigation3.html";
      var title3 = "Navigation test 3";
      loadUrl(url3);
      compareAttributes(2, 1, url3, title3, "Two entries / last one updated");
    }
  }
}
