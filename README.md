# cvgo
 computer vision : global offensive

## ???
[take a quick look here](https://youtu.be/YiGEf9hP55E).  
i implemented a way to aim and shoot in the videogame CS:GO by moving your arm in real life.  
i used my [**_vaaac library_**](https://github.com/soybin/vaaac) in order to get the arm's angle and trigger input from the user. 

## dependencies
[OpenCV](https://opencv.org/) is required in order to compile this program.

## platform
this platform uses the win32 api, which makes it specific to windows. however, it shouldn't be too hard to implement on any other platform and/or videogame, since the core logic [__*vaaac*__](https://github.com/soybin/vaaac) is platform agnostic.

## disclaimer
_this software will overwrite the game's memory in order to change the view angles and the firing state of the player in the same way that a traditional 'hack' would do it. **this software does NOT in any way provide an unfair competitive advantage of any kind to the user.** however, Valve might not like this, so use this software at your own risk_
