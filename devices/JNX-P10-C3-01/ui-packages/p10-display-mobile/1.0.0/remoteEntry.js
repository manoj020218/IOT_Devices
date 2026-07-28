(function () {
  var host = window.__JENIX_DEVICE_PACKAGE_HOST__;
  if (!host || !host.React || typeof host.registerPackage !== "function") {
    throw new Error("Jenix device package host is not available");
  }

  var React = host.React;
  var h = React.createElement;

  function readNumber(value, fallback) {
    return typeof value === "number" && isFinite(value) ? value : fallback;
  }

  function readBoolean(value, fallback) {
    return typeof value === "boolean" ? value : fallback;
  }

  function readString(value, fallback) {
    return typeof value === "string" ? value : fallback;
  }

  function readSnapshot(runtime) {
    var telemetry = runtime.telemetrySnapshot && runtime.telemetrySnapshot.telemetry
      ? runtime.telemetrySnapshot.telemetry
      : {};
    return {
      currentToken: readString(String(telemetry.currentToken != null ? telemetry.currentToken : ""), "—"),
      currentCounter: readString(String(telemetry.currentCounter != null ? telemetry.currentCounter : ""), "—"),
      displayMode: readString(telemetry.displayMode, "TOKEN"),
      displayText: readString(telemetry.displayText, ""),
      brightness: readNumber(telemetry.brightness, 35),
      announcementLanguage: readString(telemetry.announcementLanguage, "english"),
      mqttStatus: readBoolean(telemetry.mqttStatus, false),
      espNowStatus: readBoolean(telemetry.espNowStatus, false),
      peerCount: readNumber(telemetry.peerCount, 0),
      wifiRssi: readNumber(telemetry.wifiRssi, -127),
      firmwareVersion: readString(telemetry.firmwareVersion, "Unknown"),
      occurredAt: runtime.telemetrySnapshot ? runtime.telemetrySnapshot.occurredAt : ""
    };
  }

  function P10DisplayDynamicPage(props) {
    var snapshot = readSnapshot(props.runtime);

    return h("section", { className: "content-grid" }, [
      h("article", { className: "panel", key: "main" }, [
        h("div", { className: "scene-section-head", key: "head" }, h("div", {}, [
          h("span", { className: "eyebrow", key: "eyebrow" }, "Remote Package"),
          h("h2", { key: "title", style: { marginBottom: 4 } }, props.device.displayName),
          h("p", { key: "body", className: "hint-text" }, "This device page is loaded dynamically from /ui-packages.")
        ])),
        h("div", {
          key: "hero",
          style: {
            border: "1px solid rgba(15, 23, 42, 0.12)",
            borderRadius: 24,
            padding: 20,
            background: "linear-gradient(180deg, #f8fafc 0%, #e2e8f0 100%)",
            textAlign: "center"
          }
        }, [
          h("p", { className: "device-pid-label", key: "label" }, "Now Serving"),
          h("h3", { key: "value", style: { fontSize: 48, margin: 0 } }, snapshot.currentToken),
          h("p", { key: "counter", className: "hint-text" }, "Counter " + snapshot.currentCounter)
        ]),
        h("dl", { className: "summary-grid", key: "dl" }, [
          h("div", { key: "mode" }, [h("dt", { key: "dt" }, "Display Mode"), h("dd", { key: "dd" }, snapshot.displayMode)]),
          h("div", { key: "brightness" }, [h("dt", { key: "dt" }, "Brightness"), h("dd", { key: "dd" }, snapshot.brightness + "%")]),
          h("div", { key: "audio" }, [h("dt", { key: "dt" }, "Announcement Language"), h("dd", { key: "dd" }, snapshot.announcementLanguage)]),
          h("div", { key: "espnow" }, [h("dt", { key: "dt" }, "ESP-NOW"), h("dd", { key: "dd" }, snapshot.espNowStatus ? snapshot.peerCount + " peer(s)" : "Not ready")]),
          h("div", { key: "rssi" }, [h("dt", { key: "dt" }, "Wi-Fi RSSI"), h("dd", { key: "dd" }, snapshot.wifiRssi + " dBm")]),
          h("div", { key: "fw" }, [h("dt", { key: "dt" }, "Firmware"), h("dd", { key: "dd" }, snapshot.firmwareVersion)]),
          h("div", { key: "at" }, [h("dt", { key: "dt" }, "Updated"), h("dd", { key: "dd" }, snapshot.occurredAt)])
        ]),
        h("section", { className: "panel", key: "controls" }, [
          h("div", { className: "scene-section-head", key: "head" }, h("div", {}, [
            h("span", { className: "eyebrow", key: "eyebrow" }, "Actions"),
            h("h2", { key: "title", style: { marginBottom: 4 } }, "Token & Display Actions"),
            h("p", { key: "body", className: "hint-text" }, "Token, text, brightness and announcement controls live on the dedicated P10 Display panel — this quick view only shows live status.")
          ])),
          h("div", { className: "card-actions", key: "actions" }, [
            h("button", { className: "text-button", disabled: props.busy, key: "refresh", onClick: function () { void props.onRefresh(); }, type: "button" }, "Refresh Runtime")
          ])
        ])
      ])
    ]);
  }

  host.registerPackage({
    packageId: "p10-display-mobile",
    version: "1.0.0",
    exports: {
      P10DisplayDynamicPage: P10DisplayDynamicPage
    }
  });
})();
