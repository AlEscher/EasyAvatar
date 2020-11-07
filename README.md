# EasyAvatar

A TeamSpeak 3 plugin written in C.  
With this plugin you can easily set your avatar to any image from an URL.

## Usage

### Installation

1. Press `⊞ Win` + `R`
2. Type `%appdata%` and press `↵Enter`
3. In the folder that opens, navigate to `TS3Client` -> `plugins`
4. Copy EasyAvatar.dll into the folder
5. Start TeamSpeak 3
6. In TeamSpeak, if you click on `Tools` -> `Options` -> `Addons` you should see "EasyAvatar" under `Plugins`

### Context menu

Simply copy any URL of an image to your clipboard.  
When connected to a server, Right Click on yourself and at the bottom under `EasyAvatar` click `Set Image`

### Hotkey

You can bind the Setting of your Avatar to a Hot-Key.  
To do this, click on `Tools` -> `Options` -> `Hotkeys` and press `Add`.  
Click on `Show Advanced Actions`, find and open up `Plugins` -> `Plugin Hotkey` -> `EasyAvatar` -> `Set Avatar` and then set a Hotkey.  
Once you have the Hotkey set up, simply copy any URL of an image to your clipboard and press the Hotkey.


## Dependencies

I am using [FreeImage](http://freeimage.sourceforge.net) for image operations such as resizing
