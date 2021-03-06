import QtQuick 2.0
import QtTest 1.0
import Oxide.testsupport 1.0

TestWebView {
  id: webView
  focus: true

  SignalSpy {
    id: productSpy
    signalName: "productChanged"
  }
  SignalSpy {
    id: userAgentSpy
    signalName: "userAgentChanged"
  }

  Component.onCompleted: {
    productSpy.target = webView.context;
    userAgentSpy.target = webView.context;
  }

  TestCase {
    id: test
    name: "WebContext_userAgent"
    when: windowShown

    function init() {
      webView.context.product = "";
      webView.context.userAgent = "";
      productSpy.clear();
      userAgentSpy.clear();
    }

    function cleanupTestCase() {
      webView.context.product = "";
      webView.context.userAgent = "";
    }

    function makeDefaultUserAgentRegExp() {
      return new RegExp(
          "Mozilla/5\\.0 \\([^ ;\\)]+; [^ \\)]+ [^ \\)]+\\) " +
          "AppleWebKit/[0-9]+\\.[0-9]+ \\(KHTML, like Gecko\\) " +
          webView.context.product.replace(/\./g, "\\.") +
          " Safari/[0-9]+\\.[0-9]+");
    }

    function verifyUserAgent(re) {
      var userAgent = webView.context.userAgent;

      webView.url = "http://testsuite/get-headers.py"
      verify(webView.waitForLoadSucceeded(),
             "Timed out waiting for successful load");

      verify(re.test(userAgent), "Unexpected user agent string: " + userAgent);

      var headers = JSON.parse(
          webView.getTestApi().evaluateCode(
            "return document.body.children[0].innerHTML", true));
      compare(headers["user-agent"], userAgent, "Unexpected User-Agent header");

      compare(webView.getTestApi().evaluateCode(
          "return navigator.userAgent", true),
          userAgent);
    }

    function test_WebContext_userAgent1_defaults() {
      var product = webView.context.product;

      var product_re = new RegExp("Chrome/[0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+");
      verify(product_re.test(product), "Invalid product " + product);

      verifyUserAgent(makeDefaultUserAgentRegExp());
    }

    function test_WebContext_userAgent2_custom_product() {
      webView.context.product = "Oxide";
      compare(productSpy.count, 1, "Expected a signal when changing the product");
      compare(userAgentSpy.count, 1, "Expected a signal when changing the product");
      compare(webView.context.product, "Oxide",
              "Unexpected value read back from WebContext.product");

      verifyUserAgent(makeDefaultUserAgentRegExp());

      webView.context.product = webView.context.product;
      compare(productSpy.count, 1, "Shouldn't have had a signal");
      compare(userAgentSpy.count, 1, "Shouldn't have had a signal");

      webView.context.product = "";
      compare(productSpy.count, 2, "Expected a signal when changing the product");
      compare(userAgentSpy.count, 2, "Expected a signal when changing the product");

      var product = webView.context.product;
      var product_re = new RegExp("Chrome/[0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+");
      verify(product_re.test(product), "Invalid product " + product);

      verifyUserAgent(makeDefaultUserAgentRegExp());
    }

    function test_WebContext_userAgent3_custom_ua() {
      webView.context.userAgent = "Test User Agent";
      compare(userAgentSpy.count, 1, "Expected a signal when changing the user agent");
      compare(productSpy.count, 0,
              "Changing the user agent shouldn't generate a productChanged signal");

      var re = new RegExp("Test User Agent");
      verifyUserAgent(re);

      webView.context.product = "Foo";
      compare(userAgentSpy.count, 1,
              "Shouldn't have had a signal when changing the product");

      verifyUserAgent(re);

      webView.context.userAgent = "";
      compare(userAgentSpy.count, 2, "Expected a signal when changing the user agent");
      verifyUserAgent(makeDefaultUserAgentRegExp());
    }
  }
}
