import { ExtensionContext } from "@foxglove/extension";

import { initFaultsPanel } from "./FaultsPanel";

export function activate(extensionContext: ExtensionContext): void {
  extensionContext.registerPanel({ name: "Faults", initPanel: initFaultsPanel });
}
