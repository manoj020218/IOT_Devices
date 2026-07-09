import { TankGuardDynamicPage } from "./tankGuardDynamicPage";
import { buildTankGuardHomeTileModel } from "./tankGuardHomeTile";
import { tankGuardPluginManifest } from "../shared/tankGuard.plugin";

export interface TankGuardRegistryEntry {
  pluginId: string;
  pid: string;
  dynamicPageId: string;
  render: typeof TankGuardDynamicPage;
  buildHomeTile: typeof buildTankGuardHomeTileModel;
}

export const tankGuardRegistryEntry: TankGuardRegistryEntry = {
  pluginId: tankGuardPluginManifest.pluginId,
  pid: tankGuardPluginManifest.pid,
  dynamicPageId: tankGuardPluginManifest.page.dynamicPageId,
  render: TankGuardDynamicPage,
  buildHomeTile: buildTankGuardHomeTileModel
};
