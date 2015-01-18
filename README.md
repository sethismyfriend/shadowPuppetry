shadowPuppetry
==============

openframeworks app for tracking objects on a shadow screen and overlaying digital content - currently a work in progress. 

Built to work with the current version of the [OF master](https://github.com/openframeworks/openFrameworks) on Github. Requires addons listed in addons.make: 

- ofxCv: built off of the homography example on "master branch" located [here](https://github.com/kylemcdonald/ofxCv)

- ofxOpenCv: required by ofxCv - but I only include the libs. 

- ofxXmlSettings: used to save points to file and by ofxUI. Included in OF. 

- ofxUI: camera settings, homogrpahy settings. I used a fork of ofxUI located [here](https://github.com/sethismyfriend/ofxUI)

- ofxPS3EyeGrabber: pull the ["develop" branch](https://github.com/bakercp/ofxPS3EyeGrabber/tree/develop) - does not work with "master"

Here is a video showing the state as of commit 13:

<iframe src="//player.vimeo.com/video/116622619" width="500" height="281" frameborder="0" webkitallowfullscreen mozallowfullscreen allowfullscreen></iframe> <p><a href="http://vimeo.com/116622619">video on vimeo of digital pupptry prototype testing</a></p>
