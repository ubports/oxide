import QtQuick 2.0
import QtTest 1.0
import com.canonical.Oxide 1.0
import Oxide.testsupport 1.0

TestWebView {
  id: webView
  focus: true

  preferences.canRunInsecureContent: true

  onCertificateError: error.allow()

  SignalSpy {
    id: securityLevelSpy
    target: webView.securityStatus
    signalName: "securityLevelChanged"
  }
  SignalSpy {
    id: contentStatusSpy
    target: webView.securityStatus
    signalName: "contentStatusChanged"
  }
  SignalSpy {
    id: certStatusSpy
    target: webView.securityStatus
    signalName: "certStatusChanged"
  }
  SignalSpy {
    id: certificateSpy
    target: webView.securityStatus
    signalName: "certificateChanged"
  }

  TestCase {
    id: test
    name: "SecurityStatus"
    when: windowShown

    function init() {
      webView.url = "about:blank";
      verify(webView.waitForLoadSucceeded());

      webView.clearLoadEventCounters();

      securityLevelSpy.clear();
      contentStatusSpy.clear();
      certStatusSpy.clear();
      certificateSpy.clear();
    }

    function test_SecurityStatus1_data() {
      // XXX: How do we test that EV works?
      return [
        {
          url: "http://testsuite/empty.html",
          securityLevel: SecurityStatus.SecurityLevelNone,
          contentStatus: SecurityStatus.ContentStatusNormal,
          certStatus: SecurityStatus.CertStatusOk,
          securityLevelSignals: [0,0], contentStatusSignals: [0,0],
          certStatusSignals: [0,0], certificateSignals: [0,0],
          certificate: null
        },
        {
          url: "https://testsuite/empty.html",
          securityLevel: SecurityStatus.SecurityLevelSecure,
          contentStatus: SecurityStatus.ContentStatusNormal,
          certStatus: SecurityStatus.CertStatusOk,
          securityLevelSignals: [1,2], contentStatusSignals: [0,0],
          certStatusSignals: [0,0], certificateSignals: [1,2],
          certificate: "b354a8e3d1359447ec719e7a03b42cef379a4cc1"
        },
        {
          url: "https://expired.testsuite/empty.html",
          securityLevel: SecurityStatus.SecurityLevelError,
          contentStatus: SecurityStatus.ContentStatusNormal,
          certStatus: SecurityStatus.CertStatusExpired,
          securityLevelSignals: [1,2], contentStatusSignals: [0,0],
          certStatusSignals: [1,2], certificateSignals: [1,2],
          certificate: "df17c8da033e2d5ed64d1f187fdf419e1fc68e47"
        },
        {
          url: "https://selfsigned.testsuite/empty.html",
          securityLevel: SecurityStatus.SecurityLevelError,
          contentStatus: SecurityStatus.ContentStatusNormal,
          certStatus: SecurityStatus.CertStatusAuthorityInvalid,
          securityLevelSignals: [1,2], contentStatusSignals: [0,0],
          certStatusSignals: [1,2], certificateSignals: [1,2],
          certificate: "f0357f544e27adaa51211663a28cc8d64b057e63"
        },
        {
          url: "https://badidentity.testsuite/empty.html",
          securityLevel: SecurityStatus.SecurityLevelError,
          contentStatus: SecurityStatus.ContentStatusNormal,
          certStatus: SecurityStatus.CertStatusBadIdentity,
          securityLevelSignals: [1,2], contentStatusSignals: [0,0],
          certStatusSignals: [1,2], certificateSignals: [1,2],
          certificate: "89c5760286e897ad32b9dd500d70755e1e026588"
        },
        // Disabled for now because these errors are currently non-overriable
        // See https://launchpad.net/bugs/1368385
        //{
        //  url: "https://testsuite/tst_SecurityStatus_display_broken_subresource.html",
        //  securityLevel: SecurityStatus.SecurityLevelError,
        //  contentStatus: SecurityStatus.ContentStatusRanInsecure,
        //  certStatus: SecurityStatus.CertStatusOk,
        //  securityLevelSignals: [2,3], contentStatusSignals: [1,2],
        //  certStatusSignals: [0,0], certificateSignals: [1,2],
        //  certificate: "b354a8e3d1359447ec719e7a03b42cef379a4cc1"
        //},
        //{
        //  url: "https://testsuite/tst_SecurityStatus_run_broken_subresource.html",
        //  securityLevel: SecurityStatus.SecurityLevelError,
        //  contentStatus: SecurityStatus.ContentStatusRanInsecure,
        //  certStatus: SecurityStatus.CertStatusOk,
        //  securityLevelSignals: [2,3], contentStatusSignals: [1,2],
        //  certStatusSignals: [0,0], certificateSignals: [1,2],
        //  certificate: "b354a8e3d1359447ec719e7a03b42cef379a4cc1"
        //},
        //{
        //  url: "https://testsuite/tst_SecurityStatus_display_broken_subresource_in_iframe.html",
        //  securityLevel: SecurityStatus.SecurityLevelError,
        //  contentStatus: SecurityStatus.ContentStatusRanInsecure,
        //  certStatus: SecurityStatus.CertStatusOk,
        //  securityLevelSignals: [2,3], contentStatusSignals: [1,2],
        //  certStatusSignals: [0,0], certificateSignals: [1,2],
        //  certificate: "b354a8e3d1359447ec719e7a03b42cef379a4cc1"
        //},
        //{
        //  url: "https://testsuite/tst_SecurityStatus_run_broken_subresource_in_iframe.html",
        //  securityLevel: SecurityStatus.SecurityLevelError,
        //  contentStatus: SecurityStatus.ContentStatusRanInsecure,
        //  certStatus: SecurityStatus.CertStatusOk,
        //  securityLevelSignals: [2,3], contentStatusSignals: [1,2],
        //  certStatusSignals: [0,0], certificateSignals: [1,2],
        //  certificate: "b354a8e3d1359447ec719e7a03b42cef379a4cc1"
        //},
        {
          url: "https://testsuite/tst_SecurityStatus_display_insecure_content.html",
          securityLevel: SecurityStatus.SecurityLevelWarning,
          contentStatus: SecurityStatus.ContentStatusDisplayedInsecure,
          certStatus: SecurityStatus.CertStatusOk,
          securityLevelSignals: [2,3], contentStatusSignals: [1,2],
          certStatusSignals: [0,0], certificateSignals: [1,2],
          certificate: "b354a8e3d1359447ec719e7a03b42cef379a4cc1"
        },
        {
          url: "https://testsuite/tst_SecurityStatus_display_insecure_content_in_iframe.html",
          securityLevel: SecurityStatus.SecurityLevelWarning,
          contentStatus: SecurityStatus.ContentStatusDisplayedInsecure,
          certStatus: SecurityStatus.CertStatusOk,
          securityLevelSignals: [2,3], contentStatusSignals: [1,2],
          certStatusSignals: [0,0], certificateSignals: [1,2],
          certificate: "b354a8e3d1359447ec719e7a03b42cef379a4cc1"
        },
        {
          url: "https://8979847.testsuite/tst_SecurityStatus_run_insecure_content.html",
          securityLevel: SecurityStatus.SecurityLevelError,
          contentStatus: SecurityStatus.ContentStatusRanInsecure,
          certStatus: SecurityStatus.CertStatusOk,
          securityLevelSignals: [2,3], contentStatusSignals: [1,2],
          certStatusSignals: [0,0], certificateSignals: [1,2],
          certificate: "b354a8e3d1359447ec719e7a03b42cef379a4cc1"
        },
        {
          url: "https://6645227.testsuite/tst_SecurityStatus_run_insecure_content_in_iframe.html",
          securityLevel: SecurityStatus.SecurityLevelError,
          contentStatus: SecurityStatus.ContentStatusRanInsecure,
          certStatus: SecurityStatus.CertStatusOk,
          securityLevelSignals: [2,3], contentStatusSignals: [1,2],
          certStatusSignals: [0,0], certificateSignals: [1,2],
          certificate: "b354a8e3d1359447ec719e7a03b42cef379a4cc1"
        }
      ];
    }

    // This test loads content over SSL with various errors to check the behaviour
    // of the API
    function test_SecurityStatus1(data) {
      compare(webView.securityStatus.securityLevel, SecurityStatus.SecurityLevelNone);
      compare(webView.securityStatus.contentStatus, SecurityStatus.ContentStatusNormal);
      compare(webView.securityStatus.certStatus, SecurityStatus.CertStatusOk);
      compare(webView.securityStatus.certificate, null);

      // Load test URL and wait for it to load
      webView.url = data.url;
      verify(webView.waitForLoadSucceeded());

      // Verify API and signal counts
      compare(webView.securityStatus.securityLevel, data.securityLevel);
      compare(webView.securityStatus.contentStatus, data.contentStatus);
      compare(webView.securityStatus.certStatus, data.certStatus);
      if (!data.certificate) {
        compare(webView.securityStatus.certificate, null);
      } else {
        compare(webView.securityStatus.certificate.fingerprintSHA1, data.certificate);
      }
      compare(securityLevelSpy.count, data.securityLevelSignals[0]);
      compare(contentStatusSpy.count, data.contentStatusSignals[0]);
      compare(certStatusSpy.count, data.certStatusSignals[0]);
      compare(certificateSpy.count, data.certificateSignals[0]);

      // Go back to a http URL
      webView.url = "http://testsuite/empty.html";
      verify(webView.waitForLoadSucceeded());

      // Verify the API status is appropriate for a non-HTTPS URL
      compare(webView.securityStatus.securityLevel, SecurityStatus.SecurityLevelNone);
      compare(webView.securityStatus.contentStatus, SecurityStatus.ContentStatusNormal);
      compare(webView.securityStatus.certStatus, SecurityStatus.CertStatusOk);
      compare(webView.securityStatus.certificate, null);
      compare(securityLevelSpy.count, data.securityLevelSignals[1]);
      compare(contentStatusSpy.count, data.contentStatusSignals[1]);
      compare(certStatusSpy.count, data.certStatusSignals[1]);
      compare(certificateSpy.count, data.certificateSignals[1]);
    }
  }
}
