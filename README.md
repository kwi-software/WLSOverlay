# WLSOverlay - Help & Documentation

## Feature Description
**WLSOverlay** is a useful Windows service that displays a customizable overlay directly on the Windows Logon and Lock Screen.

**Key Features:**
* **Three Display Modes:**
  * **Text File:** Reads formatted text from a local file (`overlay.txt`).
  * **Image Display:** Displays a transparent image (`overlay.png`).
  * **Registry Text:** Fetches dynamic text directly from a specified Windows Registry value (ideal for scripts or system info).
* **Prefix Support:** Loads an optional `overlay_prefix.txt` file whose content is always displayed before the main text (works in Text File and Registry Text modes).
* **Live Update (Hot-Reloading):** The service monitors configuration and file paths in real-time. Changes to the text files, the image, the registry settings, or the monitored dynamic registry value are updated on the screen *instantly and without requiring a restart*.
* **Comprehensive Customization:** Flexible control over position, screen corner anchoring (Anchor), opacity, font family, font size, tab width, and text alignment. The font automatically uses a high-contrast outline (white on black / black on white) for optimal readability on any background.
* **Secure Integration:** The service runs invisibly in the background, correctly duplicates security tokens, and safely injects the overlay into the secure `WinSta0\Winlogon` desktop.

**Example Images:**

![Example 1: Text Mode](example1.png)
![Example 2: Image Mode](example2.png)

---

## Installation
1. **Place Files:** Copy the compiled executable file (e.g., `WLSOverlay.exe`) to your desired installation directory.
2. **Folder Structure:** Create a subfolder named `OverlayFiles` in the exact same directory.
3. **Add Content:** Place your display file into the `OverlayFiles` folder:
   * For Mode 0: `overlay.txt` (and optionally `overlay_prefix.txt`)
   * For Mode 1: `overlay.png`
4. **Register as a Service:** Open the Command Prompt (cmd.exe) as an Administrator and run the following command (adjust the path accordingly). The `/s` parameter tells the EXE to start in service mode:
   ```cmd
   sc create WLSOverlayService binPath= "C:\Path\To\WLSOverlay.exe /s" start= auto
   ```
5. **Start the Service:** Then, start the service using:
   ```cmd
   sc start WLSOverlayService
   ```

---

## Registry Values Description
The entire configuration of the overlay is managed via the Windows Registry.
Navigate to the key:
`HKEY_LOCAL_MACHINE\Software\KWI-Software\WLSOverlay`

*(Tip: You can change these values while the overlay is visible. The changes will be applied immediately.)*

### Configuration Values in Detail

| Value Name | Type | Description |
| :--- | :--- | :--- |
| **OverlayType** | `REG_DWORD` | **Display Mode:**<br>`0` = Text file (`overlay.txt`)<br>`1` = Image file (`overlay.png`)<br>`2` = Dynamic Registry Text |
| **Opacity** | `REG_DWORD` | Overlay opacity in percent (Values: `0` to `100`). |
| **PosX** | `REG_DWORD` | Horizontal offset (X-axis) from the chosen anchor point in pixels. |
| **PosY** | `REG_DWORD` | Vertical offset (Y-axis) from the chosen anchor point in pixels. |
| **Anchor** | `REG_DWORD` | **Screen Anchor Point:**<br>`0` = Top Left<br>`1` = Top Right<br>`2` = Bottom Left<br>`3` = Bottom Right |
| **FontSize** | `REG_DWORD` | Font size in pixels (allowed values: `6` to `149`). |
| **IsWhiteText** | `REG_DWORD` | **Text Color:**<br>`1` = White text with black outline<br>`0` = Black text with white outline |
| **TextAlignment** | `REG_DWORD` | **Text Alignment:**<br>`0` = Left-aligned<br>`1` = Centered<br>`2` = Right-aligned |
| **TabWidth** | `REG_DWORD` | Tabulator width in the text (allowed values: `1` to `32`). |
| **AutoHidePrefix** | `REG_DWORD` | **Auto Hide Behavior:**<br>`1` = Hides the overlay (including prefix) if the main text is empty.<br>`0` = Always shows the prefix, even if the main text is empty. |
| **FontFamily** | `REG_SZ` | Name of the font (e.g., `Arial`, `Consolas`, `Tahoma`). |
| **TargetRegKey** | `REG_SZ` | *(Only for OverlayType 2)*: Path of the registry key to monitor (e.g., `HKLM\Software\MyApp`). Supported roots: `HKCU`, `HKLM`, `HKCR`, `HKU`. |
| **TargetRegValue** | `REG_SZ` | *(Only for OverlayType 2)*: Name of the value (`REG_SZ` or `REG_MULTI_SZ`) in the target key whose text should be displayed. |

---

## License and Copyright
This project is licensed under the **Apache License 2.0**.

**Copyright 2026 Karl Wintermann (kwi-software)**

You may obtain a copy of the License at:
http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.

**Contact & Source:**
* GitHub: [kwi-software](https://github.com/kwi-software)
* Email: `kwi-software(at)proton.me`