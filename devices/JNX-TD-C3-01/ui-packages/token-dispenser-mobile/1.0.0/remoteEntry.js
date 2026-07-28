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
      lastPrintedToken: readString(String(telemetry.lastPrintedToken != null ? telemetry.lastPrintedToken : ""), "—"),
      printerState: readNumber(telemetry.printerState, 0),
      paperLow: readBoolean(telemetry.paperLow, false),
      paperOut: readBoolean(telemetry.paperOut, false),
      estimatedTokensLeft: readNumber(telemetry.estimatedTokensLeft, 0),
      wifiRssi: readNumber(telemetry.wifi_rssi, -127),
      firmwareVersion: readString(telemetry.firmware_version, "Unknown"),
      occurredAt: runtime.telemetrySnapshot ? runtime.telemetrySnapshot.occurredAt : ""
    };
  }

  function paperTone(estimatedTokensLeft, paperOut) {
    if (paperOut) return "#ef4444";
    if (estimatedTokensLeft <= 50) return "#ef4444";
    if (estimatedTokensLeft <= 100) return "#f59e0b";
    return "#22c55e";
  }

  function TokenDispenserDynamicPage(props) {
    var snapshot = readSnapshot(props.runtime);
    var tone = paperTone(snapshot.estimatedTokensLeft, snapshot.paperOut);

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
          h("p", { className: "device-pid-label", key: "label" }, "Current Token"),
          h("h3", { key: "value", style: { fontSize: 48, margin: 0 } }, snapshot.currentToken),
          h("p", { key: "last", className: "hint-text" }, "Last printed: " + snapshot.lastPrintedToken)
        ]),
        h("dl", { className: "summary-grid", key: "dl" }, [
          h("div", { key: "paper" }, [h("dt", { key: "dt" }, "Paper Remaining"), h("dd", { key: "dd", style: { color: tone, fontWeight: 700 } }, String(snapshot.estimatedTokensLeft) + " tokens")]),
          h("div", { key: "status" }, [h("dt", { key: "dt" }, "Printer"), h("dd", { key: "dd" }, snapshot.paperOut ? "Paper Out" : snapshot.paperLow ? "Paper Low" : "Ready")]),
          h("div", { key: "rssi" }, [h("dt", { key: "dt" }, "Wi-Fi RSSI"), h("dd", { key: "dd" }, snapshot.wifiRssi + " dBm")]),
          h("div", { key: "fw" }, [h("dt", { key: "dt" }, "Firmware"), h("dd", { key: "dd" }, snapshot.firmwareVersion)]),
          h("div", { key: "at" }, [h("dt", { key: "dt" }, "Updated"), h("dd", { key: "dd" }, snapshot.occurredAt)])
        ]),
        h("section", { className: "panel", key: "controls" }, [
          h("div", { className: "scene-section-head", key: "head" }, h("div", {}, [
            h("span", { className: "eyebrow", key: "eyebrow" }, "Actions"),
            h("h2", { key: "title", style: { marginBottom: 4 } }, "Token Actions"),
            h("p", { key: "body", className: "hint-text" }, "Print, template, and roll-reset controls live on the dedicated Token Dispenser panel — this quick view only shows live status.")
          ])),
          h("div", { className: "card-actions", key: "actions" }, [
            h("button", { className: "text-button", disabled: props.busy, key: "refresh", onClick: function () { void props.onRefresh(); }, type: "button" }, "Refresh Runtime")
          ])
        ])
      ])
    ]);
  }

  host.registerPackage({
    packageId: "token-dispenser-mobile",
    version: "1.0.0",
    exports: {
      TokenDispenserDynamicPage: TokenDispenserDynamicPage
    }
  });
})();
