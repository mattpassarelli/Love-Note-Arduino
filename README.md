# Love Note

I wanted to get a Love Box for my girlfriend before she goes to Med School, but they're $100. Each...


So I'm going to try and make my own. Simple Arduino (ESP8265 (Wemos D1 Mini Lite)) powered system that will receive messages from either my client or her's. I do want to see if I can
expand this into an app that handles more than 2 users, but I want a private version working first.

With that, I do have ideas on how to expand this into an actual product, but will require pretty significant reworks of my backend (obviously, because this is only for 2 people right now).

# How it works

The Arduino is configured to run the exact same code for both boxes, with the only differences being the specific endpoint it looks to for messages and the color of the LEDs when they pulse.

I build a custom backend and frontend UI for sending the messages and where to look for them. They're extremely simple; not trying to impress anyone with my UI skills (see streamn.live for that lol). The Wemos looks to this backend and whenever it detects a message, it will pull it.

There is a Python program included here that can convert the Certificate you pull off the page to code that you can use in the Arduino so it can pass the cert validation. This lasts much much longer than verifying the connection with a key or anything else. Might need updating in a few months, but we'll see.



The messages are formatted as JSONs, `{id: INT, message: STRING}` (format could change with new features). They Arduino pulls the image and extracts both of these fields. It compares the ID with that of the latest message pulled that is stored in memory and then displays the message if the IDs don't match. So e.g. 

```
String json = {id: 2, message: "Hello there"}; 
//parse JSON
if(id != EEPROM.read(ID_IN_MEMORY)){
 displayMessage();
}
```

The only downside I see right now is that the Arduino can only hold one byte per memory block, so about 0-255 (or 254 idk), which means I can only have about 255-256 possible IDs, meaning that my chances of the Frontend picking the exact same number after the previous message are about 1 in 254, not that great imo. I could fix it by storing the ID in multiple bytes of memory, but for something that is supposed to only be used between 2 people, it's not a huge deal. Something that will probably need to be done should I expand this to a somewhat legit product, just to guarantee no one misses a message.

# Other

I've also included the STLs I made to house these. I suppose if anyone is looking to this repo to recreate it, it might help you come up with a design, or use the current one.