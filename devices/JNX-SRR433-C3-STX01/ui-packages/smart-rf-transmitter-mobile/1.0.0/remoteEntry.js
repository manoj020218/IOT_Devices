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
      online: readBoolean(telemetry.online, false),
      wifiConnected: readBoolean(telemetry.wifiConnected, false),
      savedButtons: readNumber(telemetry.savedButtons, 0),
      commandCount: readNumber(telemetry.commandCount, 0),
      productProfile: readString(telemetry.productProfile, "GENERIC_RF_REMOTE"),
      localIp: readString(telemetry.localIp, "Unavailable"),
      firmwareVersion: readString(telemetry.firmwareVersion, "Unknown"),
      occurredAt: runtime.telemetrySnapshot ? runtime.telemetrySnapshot.occurredAt : ""
    };
  }

  function StatusDot(props) {
    return h("span", {
      style: {
        display: "inline-block",
        width: 8,
        height: 8,
        borderRadius: "50%",
        marginRight: 6,
        background: props.ok ? "#22c55e" : "#94a3b8"
      }
    });
  }

  function SmartRfTransmitterDynamicPage(props) {
    var snapshot = readSnapshot(props.runtime);

    return h("section", { className: "content-grid" }, [
      h("article", { className: "panel", key: "main" }, [
        h("div", { className: "scene-section-head", key: "head" }, h("div", {}, [
          h("span", { className: "eyebrow", key: "eyebrow" }, "Remote Package"),
          h("h2", { key: "title", style: { marginBottom: 4 } }, props.device.displayName),
          h("p", { key: "body", className: "hint-text" }, "433 MHz action bridge — RF command delivery is one-way; load status is assumed unless a separate receiver confirms it.")
        ])),
        h("dl", { className: "summary-grid", key: "dl" }, [
          h("div", { key: "online" }, [h("dt", { key: "dt" }, "Device"), h("dd", { key: "dd" }, [h(StatusDot, { key: "dot", ok: snapshot.online }), snapshot.online ? "Online" : "Offline"])]),
          h("div", { key: "wifi" }, [h("dt", { key: "dt" }, "Wi-Fi"), h("dd", { key: "dd" }, [h(StatusDot, { key: "dot", ok: snapshot.wifiConnected }), snapshot.wifiConnected ? "Connected" : "Disconnected"])]),
          h("div", { key: "buttons" }, [h("dt", { key: "dt" }, "Saved Buttons"), h("dd", { key: "dd" }, String(snapshot.savedButtons))]),
          h("div", { key: "commands" }, [h("dt", { key: "dt" }, "Commands Sent"), h("dd", { key: "dd" }, String(snapshot.commandCount))]),
          h("div", { key: "profile" }, [h("dt", { key: "dt" }, "Product Profile"), h("dd", { key: "dd" }, snapshot.productProfile)]),
          h("div", { key: "ip" }, [h("dt", { key: "dt" }, "Local IP"), h("dd", { key: "dd" }, snapshot.localIp)]),
          h("div", { key: "fw" }, [h("dt", { key: "dt" }, "Firmware"), h("dd", { key: "dd" }, snapshot.firmwareVersion)]),
          h("div", { key: "at" }, [h("dt", { key: "dt" }, "Updated"), h("dd", { key: "dd" }, snapshot.occurredAt)])
        ]),
        h("section", { className: "panel", key: "controls" }, [
          h("div", { className: "scene-section-head", key: "head" }, h("div", {}, [
            h("span", { className: "eyebrow", key: "eyebrow" }, "Actions"),
            h("h2", { key: "title", style: { marginBottom: 4 } }, "Button Management"),
            h("p", { key: "body", className: "hint-text" }, "Saved-button trigger grid, sequences, and config live on the dedicated Smart RF Transmitter panel — this quick view only shows live status.")
          ])),
          h("div", { className: "card-actions", key: "actions" }, [
            h("button", { className: "text-button", disabled: props.busy, key: "refresh", onClick: function () { void props.onRefresh(); }, type: "button" }, "Refresh Runtime")
          ])
        ])
      ])
    ]);
  }

  host.registerPackage({
    packageId: "smart-rf-transmitter-mobile",
    version: "1.0.0",
    exports: {
      SmartRfTransmitterDynamicPage: SmartRfTransmitterDynamicPage
    }
  });
})();
