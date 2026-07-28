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
      pairedRemotes: readNumber(telemetry.pairedRemotes, 0),
      activeCalls: readNumber(telemetry.activeCalls, 0),
      mode: readString(telemetry.mode, "unknown"),
      wifiConnected: readBoolean(telemetry.wifiConnected, false),
      mqttConnected: readBoolean(telemetry.mqttConnected, false),
      espNowStatus: readString(telemetry.espNowStatus, "disabled"),
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

  function NurseCallReceiverDynamicPage(props) {
    var snapshot = readSnapshot(props.runtime);
    var activeCalls = snapshot.activeCalls;

    return h("section", { className: "content-grid" }, [
      h("article", { className: "panel", key: "main" }, [
        h("div", { className: "scene-section-head", key: "head" }, h("div", {}, [
          h("span", { className: "eyebrow", key: "eyebrow" }, "Remote Package"),
          h("h2", { key: "title", style: { marginBottom: 4 } }, props.device.displayName),
          h("p", { key: "body", className: "hint-text" }, "Nurse call receiver — remotes and active calls page loaded dynamically from /ui-packages.")
        ])),
        h("div", {
          key: "hero",
          style: {
            border: "1px solid rgba(15, 23, 42, 0.12)",
            borderRadius: 24,
            padding: 20,
            background: activeCalls > 0
              ? "linear-gradient(180deg, #fee2e2 0%, #fef2f2 100%)"
              : "linear-gradient(180deg, #f0fdf4 0%, #f8fafc 100%)"
          }
        }, [
          h("p", { className: "device-pid-label", key: "label" }, "Active Calls"),
          h("h3", { key: "value", style: { fontSize: 40, margin: 0 } }, String(activeCalls)),
          h("p", { key: "hint", className: "hint-text" },
            activeCalls > 0
              ? "Open the calls list from the device drawer to attend."
              : "No active calls right now.")
        ]),
        h("dl", { className: "summary-grid", key: "dl" }, [
          h("div", { key: "remotes" }, [h("dt", { key: "dt" }, "Paired Remotes"), h("dd", { key: "dd" }, String(snapshot.pairedRemotes))]),
          h("div", { key: "mode" }, [h("dt", { key: "dt" }, "Wi-Fi Mode"), h("dd", { key: "dd" }, snapshot.mode)]),
          h("div", { key: "wifi" }, [h("dt", { key: "dt" }, "Wi-Fi"), h("dd", { key: "dd" }, [h(StatusDot, { key: "dot", ok: snapshot.wifiConnected }), snapshot.wifiConnected ? "Connected" : "Disconnected"])]),
          h("div", { key: "mqtt" }, [h("dt", { key: "dt" }, "MQTT"), h("dd", { key: "dd" }, [h(StatusDot, { key: "dot", ok: snapshot.mqttConnected }), snapshot.mqttConnected ? "Connected" : "Disconnected"])]),
          h("div", { key: "espnow" }, [h("dt", { key: "dt" }, "ESP-NOW"), h("dd", { key: "dd" }, snapshot.espNowStatus)]),
          h("div", { key: "at" }, [h("dt", { key: "dt" }, "Updated"), h("dd", { key: "dd" }, snapshot.occurredAt)])
        ]),
        h("section", { className: "panel", key: "controls" }, [
          h("div", { className: "scene-section-head", key: "head" }, h("div", {}, [
            h("span", { className: "eyebrow", key: "eyebrow" }, "Actions"),
            h("h2", { key: "title", style: { marginBottom: 4 } }, "Receiver Controls"),
            h("p", { key: "body", className: "hint-text" }, "Active calls, remotes, and attended history live on the dedicated nurse-call panel (see the device drawer).")
          ])),
          h("div", { className: "card-actions", key: "actions" }, [
            h("button", { className: "text-button", disabled: props.busy, key: "refresh", onClick: function () { void props.onRefresh(); }, type: "button" }, "Refresh Runtime"),
            h("button", { className: "text-button", disabled: props.busy, key: "learn", onClick: function () { void props.onCommand({ command: "start_learning", requiresAck: true }); }, type: "button" }, "Learn New Remote"),
            h("button", { className: "text-button", disabled: props.busy, key: "restart", onClick: function () { void props.onCommand({ command: "restart", requiresAck: true }); }, type: "button" }, "Restart Receiver")
          ])
        ])
      ])
    ]);
  }

  host.registerPackage({
    packageId: "nurse-call-receiver-mobile",
    version: "1.0.0",
    exports: {
      NurseCallReceiverDynamicPage: NurseCallReceiverDynamicPage
    }
  });
})();
