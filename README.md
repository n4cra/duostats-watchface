# DuoStats Watchface

A custom Pebble watchface for the **Pebble 2** (and all other Pebble color platforms) built with Pebble SDK 4.5 in C.

## Features

- **Large time display** - HH:MM in the top third of the screen using the LECO-42 numeric font, vertically and horizontally centred
- **Four configurable data boxes** - two rows of two, each independently assignable to any of:
  - Steps (today's total step count)
  - Distance (metres walked today)
  - Calories (active + resting, kcal)
  - Sleep (hours and minutes from midnight)
- **Dark and light mode** - toggle via the settings page
- **Persistent settings** - all choices survive a reboot
- **Health auto-refresh** - data updates on every health event and every 5 minutes

## Layout

```
+------------------+
|                  |
|     10:10        |  <- top 1/3  (time, vertically + horizontally centred)
|                  |
+------------------+
|  Box A  | Box B  |  <- row 1
+---------+---------+
|  Box C  | Box D  |  <- row 2
+------------------+
```

Default box assignment:

| Position | Default |
|---|---|
| A - top left | Steps |
| B - top right | Distance (m) |
| C - bottom left | Calories |
| D - bottom right | Sleep |

## Supported Platforms

| Platform | Watch |
|---|---|
| `diorite` | Pebble 2 |
| `basalt` | Pebble Time |
| `chalk` | Pebble Time Round |
| `emery` | Pebble Time 2 |
| `aplite` | Original Pebble (no health data) |

---

## Installing the Pre-built Release

1. Download `duostats.pbw` from the [latest release](https://github.com/n4cra/duostats-watchface/releases/latest)
2. On Android: open the `.pbw` file with the Pebble / Rebble app
3. On iOS: AirDrop or share the file to the Pebble app

---

## Building from Source

### Prerequisites

- Python 3
- [Pebble SDK 4.5](https://developer.rebble.io/developer.pebble.com/sdk/index.html) via `pebble-tool`
- `arm-none-eabi-gcc` (ARM cross-compiler)

### Install dependencies

```bash
# Install pebble-tool
pip install pebble-tool

# Install ARM cross-compiler (Debian/Ubuntu)
sudo apt-get install gcc-arm-none-eabi

# Install Pebble SDK
pebble sdk install 4.5
```

### Build

```bash
git clone https://github.com/n4cra/duostats-watchface.git
cd duostats-watchface
pebble build
```

The compiled bundle will be at `build/duostats.pbw`.

### Install to watch

```bash
# Over Wi-Fi (phone and watch on same network)
pebble install --phone <your-phone-ip>

# Or copy build/duostats.pbw to your phone and open with the Pebble app
```

---

## Settings / Configuration Page

The watchface includes a settings page (`config/config.html`) that opens from the Pebble app's watchface settings button. It provides:

- A live interactive watch preview that updates as you change settings
- Dark / light mode toggle
- Independent data source selector for each of the four boxes

### Hosting the config page

The config page must be served over HTTPS. The easiest option is GitHub Pages:

1. Go to **Settings > Pages** on this repo, set source to `main` branch, `/ (root)`
2. The config page will then be live at:
   `https://n4cra.github.io/duostats-watchface/config/config.html`
3. The `src/pkjs/index.js` is already pre-configured to point to this URL - no code changes needed after enabling Pages
4. Run `pebble build` and reinstall the `.pbw`

---

## Project Structure

```
duostats-watchface/
+-- src/
|   +-- c/
|   |   +-- main.c          <- watchface C source
|   +-- pkjs/
|       +-- index.js        <- phone-side JS config bridge
+-- config/
|   +-- config.html         <- settings page (host on any web server)
+-- package.json            <- Pebble app manifest and message keys
+-- wscript                 <- waf build script
+-- README.md
```

## Data Source IDs

| ID | Metric |
|---|---|
| 0 | Steps |
| 1 | Distance (metres) |
| 2 | Calories |
| 3 | Sleep |

## License

MIT - free to use, modify, and distribute.
