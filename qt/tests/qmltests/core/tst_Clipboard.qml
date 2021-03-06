import QtQuick 2.0
import QtTest 1.0
import com.canonical.Oxide 1.0
import Oxide.testsupport 1.0

TestWebView {
  id: webView
  focus: true

  function expect_content(expected, rangemax) {
    var result = webView.getTestApi().evaluateCode(
        "return document.querySelector('#content').value", true);
      if (!rangemax) {
        rangemax = -1
      }
      return String(result).substr(0, rangemax) === expected.substr(0, rangemax);
    }

  function expect_has_file(expected) {
    var result = webView.getTestApi().evaluateCode(
        "return document.querySelector('#has_file').innerHTML", true);
    if (result == 'true') {
      result = true
    } else if (result == 'false') {
      result = false
    } else {
      return false
    }
    return (result === expected);
  }

  function expect_mime_type(expected) {
    var result = webView.getTestApi().evaluateCode(
      "return document.querySelector('#mime_type').innerHTML", true);
      return (result === expected);
  }

  function select_textarea_content() {
    webView.getTestApi().evaluateCode(
        "document.querySelector('#content').select()", true);
  }

  function set_content(content) {
    webView.getTestApi().evaluateCode(
        "document.querySelector('#content').value = '" + content + "'", true);
  }

  function get_image_data() {
    return "iVBORw0KGgoAAAANSUhEUgAAAAIAAAACCAIAAAD91JpzAAAACXBIWXMAAAsTAAALEwEAmpwYAAAAGUlEQVQIHQEOAPH/AAAAAAAAAAAAAAAAAAAADgABd6ch7AAAAABJRU5ErkJggg=="
  }

  TestCase {
    id: testcase
    name: "clipboard"

    when: windowShown

    function setup() {
      ClipboardTestUtils.clearClipboard();
    }

    function test_paste_data() {
      return [
          { content: "content", mimeType: "text/plain" },
          { content: get_image_data(), mimeType: "image/png"},
          { content: "<div><a href=\"#\">Test</a></div>", mimeType: "text/html"},
        ];
    }

    function test_paste(data) {
      var isImage = (data.mimeType.indexOf("image/") === 0)

      ClipboardTestUtils.copyToClipboard(data.mimeType, data.content);

      webView.url = "http://testsuite/tst_Clipboard.html";

      verify(webView.waitForLoadSucceeded(), "Timed out waiting for successful load");

      if ( ! isImage) {
        select_textarea_content();
      }
      keyPress("v", Qt.ControlModifier)

      verify(TestUtils.waitFor(function() { return expect_mime_type(data.mimeType); }));
      verify(TestUtils.waitFor(function() { return expect_has_file(isImage); }));

      if (isImage) {
        /**
         The image we get from QImage and the one we pasted are slightly different
         but overall is the same image. QImage does ARGB -> RGBA conversion
         on the raw image content
         that slightly alters it and make it hard to have an exact match.
         
         The '34' magic number in this case has been empirically determined as the
         maximum amount of content that stays the same after the conversion in this
         context.
         */
        verify(TestUtils.waitFor(function() { return expect_content(data.content, 34); }));
      } else {
        verify(TestUtils.waitFor(function() { return expect_content(data.content); }));
      }
    }

    function test_copy() {
      webView.url = "http://testsuite/tst_Clipboard.html";

      verify(webView.waitForLoadSucceeded(), "Timed out waiting for successful load");

      var data_content = "content"
      set_content(data_content)

      TestUtils.waitFor(function() { return expect_content(data_content); });

      select_textarea_content();

      keyPress("c", Qt.ControlModifier)

      verify(TestUtils.waitFor(function() {
        var current_content = ClipboardTestUtils.getFromClipboard(
            "text/plain");
        return current_content === data_content
      }));
    }
  }
}
