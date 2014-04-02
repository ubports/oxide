import QtQuick 2.0
import QtTest 1.0
import com.canonical.Oxide 1.0
import com.canonical.Oxide.Testing 1.0 as Testing

Item {
  SignalSpy {
    id: spy
    target: Oxide
  }

  Component {
    id: webView
    WebView {}
  }

  TestCase {
    id: test
    name: "OxideGlobal"
    when: windowShown

    function init() {
      spy.clear();
      spy.signalName = "";
    }

    function test_OxideGlobal_data() {
      var r = [
        { attr: "product", signal: "productChanged", value1: "Test1", value2: "Test2", constructOnly: false },
        { attr: "userAgent", signal: "userAgentChanged", value1: "Test1", value2: "Test2", contructOnly: false },
        { attr: "acceptLangs", signal: "acceptLangsChanged", value1: "foo", value2: "bar", constructOnly: false }
      ];

      var dataPath = Testing.OxideTestingUtils.DATA_PATH;
      if (dataPath != "") {
        r.push(
            { attr: "dataPath", signal: "dataPathChanged", value1: dataPath + "/test1", value2: dataPath + "/test2", constructOnly: true },
            { attr: "cachePath", signal: "cachePathChanged", value1: dataPath + "/test1", value2: dataPath + "/test2", constructOnly: true }
        );
      }

      return r;
    }

    function test_OxideGlobal(data) {
      spy.signalName = data.signal;

      Oxide[data.attr] = data.value1;
      compare(spy.count, 1,
              "Setting attribute before context exists should have generated a signal");
      compare(Oxide[data.attr], data.value1,
              "Got the wrong value back");

      var view = webView.createObject(null, {});
      compare(Oxide._defaultWebContext[data.attr], data.value1,
              "Got the wrong value back from the default context");

      var expectedCount = 1;
      var expectedVal;

      Oxide[data.attr] = data.value2;
      expectedVal = data.constructOnly ? data.value1 : data.value2;
      if (!data.constructOnly) {
        expectedCount++;
      }
      compare(spy.count, expectedCount, "Unexpected number of signals");
      compare(Oxide[data.attr], expectedVal,
              "Got the wrong value back");
      compare(Oxide._defaultWebContext[data.attr], expectedVal,
              "Got the wrong value back from the default context");

      if (!data.constructOnly) {
        Oxide._defaultWebContext[data.attr] = data.value1;
        compare(spy.count, expectedCount + 1,
                "Setting attribute on the default context should generate a signal");
        compare(Oxide[data.attr], data.value1,
                "Got the wrong value back");
      }

      Testing.OxideTestingUtils.destroyQObjectNow(view);
      verify(!Oxide._defaultWebContext);

      compare(Oxide[data.attr], data.value1,
              "Got the wrong value back");
    }
  }
}
