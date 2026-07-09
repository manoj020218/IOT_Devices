export const tankGuardUiPackageDescriptor = {
  packageId: "tank-guard-mobile",
  version: "1.0.0",
  manifestPath: "/ui-packages/tank-guard-mobile/1.0.0/manifest.json",
  entryPath: "/ui-packages/tank-guard-mobile/1.0.0/remoteEntry.js",
  exportName: "TankGuardDynamicPage",
  supportedPids: ["JNX-TG-C3-001"],
  supportedDynamicPages: ["tank-level", "thresholds", "tank-guard-mobile"]
} as const;
