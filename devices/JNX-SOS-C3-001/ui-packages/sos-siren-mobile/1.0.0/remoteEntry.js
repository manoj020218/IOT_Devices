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
      sirenState: readString(telemetry.sirenState, "UNKNOWN"),
      sosActive: readBoolean(telemetry.sosActive, false),
      sosPressCount: readNumber(telemetry.sosPressCount, 0),
      activeDutyPercent: readNumber(telemetry.activeDutyPercent, 0),
      activeFrequencyHz: readNumber(telemetry.activeFrequencyHz, 0),
      selectedTone: readString(telemetry.selectedTone, "Unknown"),
      speakerProfile: readString(telemetry.speakerProfile, "Unknown"),
      vtTriggerEnabled: readBoolean(telemetry.vtTriggerEnabled, false),
      vtTriggerHigh: readBoolean(telemetry.vtTriggerHigh, false),
      staConnected: readBoolean(telemetry.staConnected, false),
      firmwareVersion: readString(telemetry.firmwareVersion, "Unknown"),
      occurredAt: runtime.telemetrySnapshot ? runtime.telemetrySnapshot.occurredAt : ""
    };
  }

  function stateTone(sosActive, sirenState) {
    if (sosActive || sirenState === "ALARM") return "#ef4444";
    if (sirenState === "TEST") return "#f59e0b";
    return "#22c55e";
  }

  function SosSirenDynamicPage(props) {
    var snapshot = readSnapshot(props.runtime);
    var tone = stateTone(snapshot.sosActive, snapshot.sirenState);

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
          h("p", { className: "device-pid-label", key: "label" }, "Siren State"),
          h("h3", { key: "value", style: { fontSize: 32, margin: 0, color: tone } }, snapshot.sosActive ? "ALARM ACTIVE" : snapshot.sirenState),
          h("p", { key: "presses", className: "hint-text" }, "SOS presses on device: " + snapshot.sosPressCount)
        ]),
        h("dl", { className: "summary-grid", key: "dl" }, [
          h("div", { key: "tone" }, [h("dt", { key: "dt" }, "Selected Tone"), h("dd", { key: "dd" }, snapshot.selectedTone)]),
          h("div", { key: "speaker" }, [h("dt", { key: "dt" }, "Speaker Profile"), h("dd", { key: "dd" }, snapshot.speakerProfile)]),
          h("div", { key: "duty" }, [h("dt", { key: "dt" }, "Active Duty"), h("dd", { key: "dd" }, snapshot.activeDutyPercent + "%")]),
          h("div", { key: "vt" }, [h("dt", { key: "dt" }, "VT Trigger"), h("dd", { key: "dd" }, snapshot.vtTriggerEnabled ? (snapshot.vtTriggerHigh ? "Enabled, currently high" : "Enabled, idle") : "Disabled")]),
          h("div", { key: "wifi" }, [h("dt", { key: "dt" }, "Wi-Fi"), h("dd", { key: "dd" }, snapshot.staConnected ? "Connected" : "Not connected")]),
          h("div", { key: "fw" }, [h("dt", { key: "dt" }, "Firmware"), h("dd", { key: "dd" }, snapshot.firmwareVersion)]),
          h("div", { key: "at" }, [h("dt", { key: "dt" }, "Updated"), h("dd", { key: "dd" }, snapshot.occurredAt)])
        ]),
        h("section", { className: "panel", key: "controls" }, [
          h("div", { className: "scene-section-head", key: "head" }, h("div", {}, [
            h("span", { className: "eyebrow", key: "eyebrow" }, "Actions"),
            h("h2", { key: "title", style: { marginBottom: 4 } }, "Alarm Controls"),
            h("p", { key: "body", className: "hint-text" }, "Trigger, stop, tone-test and speaker-profile controls live on the dedicated SOS Siren panel — this quick view only shows live status.")
          ])),
          h("div", { className: "card-actions", key: "actions" }, [
            h("button", { className: "text-button", disabled: props.busy, key: "refresh", onClick: function () { void props.onRefresh(); }, type: "button" }, "Refresh Runtime")
          ])
        ])
      ])
    ]);
  }

  host.registerPackage({
    packageId: "sos-siren-mobile",
    version: "1.0.0",
    exports: {
      SosSirenDynamicPage: SosSirenDynamicPage
    }
  });
})();
