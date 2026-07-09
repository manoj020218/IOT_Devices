(function () {
  var host = window.__JENIX_DEVICE_PACKAGE_HOST__;
  if (!host || !host.React || typeof host.registerPackage !== "function") {
    throw new Error("Jenix device package host is not available");
  }

  var React = host.React;
  var h = React.createElement;

  function isRecord(value) {
    return typeof value === "object" && value !== null && !Array.isArray(value);
  }

  function readNumber(value, fallback) {
    return typeof value === "number" && isFinite(value) ? value : fallback;
  }

  function readBoolean(value, fallback) {
    return typeof value === "boolean" ? value : fallback;
  }

  function readString(value, fallback) {
    return typeof value === "string" ? value : fallback;
  }

  function readSettings(runtime) {
    var settings = isRecord(runtime.settings) ? runtime.settings : {};
    var config = isRecord(settings.config) ? settings.config : {};
    var alarm = isRecord(settings.alarm) ? settings.alarm : {};
    return {
      config: {
        capacityLitres: readNumber(config.capacityLitres, 1000),
        wifiTxPowerDbm: readNumber(config.wifiTxPowerDbm, 8.5),
        zeroLevelMm: readNumber(config.zeroLevelMm, 0)
      },
      alarm: {
        repeatEnabled: readBoolean(alarm.repeatEnabled, true),
        lowLevelPct: readNumber(alarm.lowLevelPct, 20)
      }
    };
  }

  function readSnapshot(runtime, settings) {
    var telemetry = runtime.telemetrySnapshot && runtime.telemetrySnapshot.telemetry
      ? runtime.telemetrySnapshot.telemetry
      : {};
    return {
      levelPct: readNumber(telemetry.tankLevelPct, 0),
      waterLevelMm: readNumber(telemetry.tankLevelMm, 0),
      zeroLevelMm: readNumber(telemetry.zeroLevelMm, settings.config.zeroLevelMm),
      flowLitresPerMin: readNumber(telemetry.flowLitresPerMin, 0),
      rssiDbm: readNumber(telemetry.wifiRssi, -127),
      wifiSsid: readString(telemetry.wifiSsidName, "Unavailable"),
      localIp: readString(telemetry.localIp, "Unavailable"),
      localUrl: readString(telemetry.localUrl, "Unavailable"),
      wifiTxPowerDbm: readNumber(telemetry.wifiTxPowerDbm, settings.config.wifiTxPowerDbm),
      pumpRunning: readBoolean(telemetry.pumpRunning, false),
      alarmState: readString(telemetry.alarmState, "normal"),
      sensorStatus: readString(telemetry.sensorStatus, "unknown"),
      occurredAt: runtime.telemetrySnapshot ? runtime.telemetrySnapshot.occurredAt : ""
    };
  }

  function updateDraft(current, section, field, value) {
    var next = Object.assign({}, current);
    next[section] = Object.assign({}, current[section], {});
    next[section][field] = value;
    return next;
  }

  function SettingsPanel(props) {
    function numberField(section, field, label) {
      return h("label", { key: section + field, style: { display: "grid", gap: 6 } }, [
        h("span", { key: "label", style: { color: "var(--muted)", fontSize: 12 } }, label),
        h("input", {
          key: "input",
          type: "number",
          value: props.value[section][field],
          onChange: function (event) {
            props.onChange(updateDraft(props.value, section, field, Number(event.target.value)));
          }
        })
      ]);
    }

    return h("section", { className: "panel" }, [
      h("div", { className: "scene-section-head", key: "head" }, h("div", {}, [
        h("span", { className: "eyebrow", key: "eyebrow" }, "Detail Settings"),
        h("h2", { key: "title", style: { marginBottom: 4 } }, "Config and Alarm"),
        h("p", { key: "body", className: "hint-text" }, "Changes are published from the shell to the device command channel.")
      ])),
      h("div", { className: "summary-grid", key: "grid" }, [
        numberField("config", "capacityLitres", "Capacity (L)"),
        numberField("config", "wifiTxPowerDbm", "WiFi TX (dBm)"),
        numberField("config", "zeroLevelMm", "Zero Level (mm)"),
        numberField("alarm", "lowLevelPct", "Low Level %")
      ]),
      h("div", { className: "card-actions", key: "actions", style: { marginTop: 16 } }, [
        h("button", { className: "text-button", key: "close", onClick: props.onClose, type: "button" }, "Close"),
        h("button", { className: "text-button", disabled: props.busy, key: "save", onClick: props.onSave, type: "button" }, props.busy ? "Saving..." : "Save Settings")
      ])
    ]);
  }

  function TankGuardDynamicPage(props) {
    var settings = readSettings(props.runtime);
    var snapshot = readSnapshot(props.runtime, settings);
    var state = React.useState(settings);
    var draft = state[0];
    var setDraft = state[1];
    var toggleState = React.useState(false);
    var showSettings = toggleState[0];
    var setShowSettings = toggleState[1];

    React.useEffect(function () {
      setDraft(settings);
    }, [props.runtime]);

    return h("section", { className: "content-grid" }, [
      h("article", { className: "panel", key: "main" }, [
        h("div", { className: "scene-section-head", key: "head" }, h("div", {}, [
          h("span", { className: "eyebrow", key: "eyebrow" }, "Remote Package"),
          h("h2", { key: "title", style: { marginBottom: 4 } }, props.device.displayName),
          h("p", { key: "body", className: "hint-text" }, "This device page is loaded dynamically from /ui-packages.")
        ])),
        h("div", { key: "hero", style: { border: "1px solid rgba(15, 23, 42, 0.12)", borderRadius: 24, padding: 20, background: "linear-gradient(180deg, #f8fafc 0%, #e2e8f0 100%)" } }, [
          h("div", { key: "tank", style: { height: 280, borderRadius: 28, overflow: "hidden", position: "relative", background: "linear-gradient(180deg, #dbeafe 0%, #eff6ff 100%)" } }, [
            h("div", { key: "fill", style: { position: "absolute", inset: "auto 0 0 0", height: Math.max(0, Math.min(100, snapshot.levelPct)) + "%", background: "linear-gradient(180deg, #38bdf8 0%, #0ea5e9 100%)", transition: "height 300ms ease" } }),
            h("div", { key: "copy", style: { position: "absolute", inset: 20, display: "grid", alignContent: "space-between" } }, [
              h("div", { key: "top" }, [
                h("p", { className: "device-pid-label", key: "label" }, "Water Level"),
                h("h3", { key: "value", style: { fontSize: 40, margin: 0 } }, snapshot.levelPct.toFixed(0) + "%")
              ]),
              h("div", { className: "summary-grid", key: "stats" }, [
                h("div", { key: "mm" }, [h("dt", { key: "dt" }, "Level (mm)"), h("dd", { key: "dd" }, String(snapshot.waterLevelMm))]),
                h("div", { key: "pump" }, [h("dt", { key: "dt" }, "Pump"), h("dd", { key: "dd" }, snapshot.pumpRunning ? "Running" : "Idle")]),
                h("div", { key: "sensor" }, [h("dt", { key: "dt" }, "Sensor"), h("dd", { key: "dd" }, snapshot.sensorStatus)]),
                h("div", { key: "zero" }, [h("dt", { key: "dt" }, "Zero"), h("dd", { key: "dd" }, String(snapshot.zeroLevelMm))])
              ])
            ])
          ])
        ]),
        h("dl", { className: "summary-grid", key: "dl" }, [
          h("div", { key: "rssi" }, [h("dt", { key: "dt" }, "WiFi RSSI"), h("dd", { key: "dd" }, snapshot.rssiDbm + " dBm")]),
          h("div", { key: "flow" }, [h("dt", { key: "dt" }, "Flow"), h("dd", { key: "dd" }, snapshot.flowLitresPerMin + " L/min")]),
          h("div", { key: "ssid" }, [h("dt", { key: "dt" }, "SSID"), h("dd", { key: "dd" }, snapshot.wifiSsid)]),
          h("div", { key: "ip" }, [h("dt", { key: "dt" }, "Local IP"), h("dd", { key: "dd" }, snapshot.localIp)]),
          h("div", { key: "url" }, [h("dt", { key: "dt" }, "Local URL"), h("dd", { key: "dd" }, snapshot.localUrl)]),
          h("div", { key: "tx" }, [h("dt", { key: "dt" }, "WiFi TX"), h("dd", { key: "dd" }, snapshot.wifiTxPowerDbm + " dBm")]),
          h("div", { key: "alarm" }, [h("dt", { key: "dt" }, "Alarm"), h("dd", { key: "dd" }, snapshot.alarmState)]),
          h("div", { key: "at" }, [h("dt", { key: "dt" }, "Updated"), h("dd", { key: "dd" }, snapshot.occurredAt)])
        ]),
        h("section", { className: "panel", key: "controls" }, [
          h("div", { className: "scene-section-head", key: "head" }, h("div", {}, [
            h("span", { className: "eyebrow", key: "eyebrow" }, "Calibration"),
            h("h2", { key: "title", style: { marginBottom: 4 } }, "Zero and Capacity Controls"),
            h("p", { key: "body", className: "hint-text" }, "Capacity is " + draft.config.capacityLitres + " L and TX power defaults to 8.5 dBm.")
          ])),
          h("div", { className: "card-actions", key: "actions" }, [
            h("button", { className: "text-button", disabled: props.busy, key: "refresh", onClick: function () { void props.onRefresh(); }, type: "button" }, "Refresh Runtime"),
            h("button", { className: "text-button", disabled: props.busy, key: "zero", onClick: function () { void props.onCommand({ command: "zero_calibrate", requiresAck: true }); }, type: "button" }, "Zero Calibrate"),
            h("button", { className: "text-button", disabled: props.busy, key: "settings", onClick: function () { setShowSettings(!showSettings); }, type: "button" }, showSettings ? "Hide Settings" : "Detail Settings")
          ])
        ])
      ]),
      showSettings ? h(SettingsPanel, {
        key: "settings",
        busy: props.busy,
        value: draft,
        onChange: setDraft,
        onClose: function () { setShowSettings(false); },
        onSave: function () {
          void props.onCommand({ command: "apply_settings", requiresAck: true, payload: { schemaVersion: 1, settings: draft } });
          setShowSettings(false);
        }
      }) : null
    ]);
  }

  host.registerPackage({
    packageId: "tank-guard-mobile",
    version: "1.0.0",
    exports: {
      TankGuardDynamicPage: TankGuardDynamicPage
    }
  });
})();
