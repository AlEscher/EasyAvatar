# EasyAvatar
[![Release](https://img.shields.io/github/v/release/AlEscher/EasyAvatar?color=brightgreen&label=Download&style=plastic)](https://github.com/AlEscher/EasyAvatar/releases/latest/download/EasyAvatar.dll)
[![GitHub](https://img.shields.io/github/license/AlEscher/EasyAvatar?color=blue&style=plastic)](https://github.com/AlEscher/EasyAvatar/blob/master/LICENSE)

A TeamSpeak 3 plugin written in C.  
With this plugin you can easily set your avatar to any image from an URL.

## Installation

1. Press `⊞ Win` + `R`
2. Type `%appdata%` and press `↵Enter`
3. In the folder that opens, navigate to `TS3Client` 🠖 `plugins`
4. Copy EasyAvatar.dll into the folder
5. Start TeamSpeak 3
6. In TeamSpeak, if you click on `Tools` 🠖 `Options` 🠖 `Addons` you should see "EasyAvatar" under `Plugins`

## Usage

The URL you provide should point directly to an image, i.e. it should end with e.g. `.png` or `.jpeg`.  
You can get it by right clicking on any image in your browser and selecting "Copy Image **Link**"

### Using a Hotkey

You can bind the Setting of your Avatar to a Hot-Key.  
1. Click on `Tools` 🠖 `Options` 🠖 `Hotkeys` and press `Add`.  
2. Click on `Show Advanced Actions`  
3. Find and open up `Plugins` 🠖 `Plugin Hotkey` 🠖 `EasyAvatar` 🠖 `Set Avatar` and then set a Hotkey.

Once you have the Hotkey set up, simply copy any URL of an image to your clipboard and press the Hotkey.

### Using the Context Menu

Simply copy any URL of an image to your clipboard.  
When connected to a server, Right Click on yourself and at the bottom under `EasyAvatar` click `Set Image`


## Dependencies

I am using [FreeImage 3.18](http://freeimage.sourceforge.net) for image operations such as resizing
