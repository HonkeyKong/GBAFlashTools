## GBA Flash Tools

This program can take a GBA ROM, and flash it to one of those cheap bootleg GBA carts you can find everywhere on AliExpress. Right now, the 8MB (Link to the Past) and 16MB (Minish Cap/Pokemon) cartridges are supported. 32MB carts will probably come after I get my hands on one.

To build the flashing client, just run this from the project root:  

```
cd FlashClient
mkdir build
cd build
arm-none-eabi-cmake ..
make
```

Now you've got a `flashclient.gba` file that you can either load from a flashcart or via multiboot, and write to your bootleg cartridge.

To use the server program, simply copy your ROM to the `FlashServer/res` folder as `rom.gba`, and then do this from your project root:  

  ```
  cd FlashServer
  mkdir build
  cd build
  arm-none-eabi-cmake ..
  make
  ```

Once you've done this, you should have a `flashserver.gba` file that you can use to write your game to a cartridge. The simplest workflow for flashing a game to a bootleg cartridge is like so:  
  
  * Load `flashclient.gba` on the receiving GBA, whether via a flashcart or multiboot.
  * Insert the bootleg cartridge you're flashing and press A.
  * Connect a GBC (not GBA!) link cable between the sending and receiving GBA
  * Start `flashserver.gba` on the sending console.
  
Now, just sit back and wait. This method isn't the fastest, at around 2.5 minutes per megabyte transferred, but it's good for those who lack specialized flashing hardware, yet still have a way of running code on the Game Boy Advance. I figure this covers most homebrew developers.
