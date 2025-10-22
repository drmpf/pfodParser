# pfodWebServer
These are the javascript/html files needed to provide pfodWeb in a web browser. 

The dist directory, contains all these files combined into single loadable files. No local server is required. 
No additional downloads are required. You can run pfodWeb on a completely isolated network with no internet access.

build.bat and build.sh builds the combined files and updates the dist directory.

index.html prompts for the IP address of the pfoDevice that will respond to http requests for pfod messages.  
pfodWeb.html requests the pfod menu/dwg and displays it.  
pfodWebDebug.html is the debug version with extensive logging in the browser console and which makes the touchZones outlines visible.  
Use pfodWeb.html?targetIP=...  directly to connect the the pfodDevice with out going via index.html.  

These files can also be served from the pfodDevice itself if there is a file system with sufficient space less than 200kB.  

# How-To
See [pfodWeb Installation and Tutorials](https://www.forward.com.au/pfod/pfodWeb/index.html)  

# Security and Privacy
pfodWebServer does not use any third party code and does not require any internet access so you can run on a completely isolated network.  

# Software License
(c)2014-2025 Forward Computing and Control Pty. Ltd.  
NSW Australia, www.forward.com.au  
This code is not warranted to be fit for any purpose. You may only use it at your own risk.  
This code may be freely used for both private and commercial use  
Provide this copyright is maintained.  

# Revisions
Version 2.0.0 removed nodejs server from pfodWebServer and bundled all files in single html files  
Version 1.1.5 used gzip files to reduce microprocessor file storage requirements  
Version 1.1.3 drawing updates as response received and included dependent node packages and removed package install script from batch files  
Version 1.1.2 fixed hiding of touchActionInput labels  
Version 1.1.1 fixed loss of idx on edit  
Version 1.1.0 fix hide/unhide and other general improvements  
Version 1.0.2 fix for drag touchActions  
Version 1.0.1 fix for debug display and mainmenu updates  
Version 1.0.0 initial release  

