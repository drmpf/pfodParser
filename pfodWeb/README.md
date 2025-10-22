# pfodWeb
This directory contains all the html and js files combined in to single loadable files. No local server is required. 
No additional downloads are required. You can run pfodWeb on a completely isolated network with no internet access.  

index.html prompts to use either pfodWeb or pfodWebDebug.  pfodWebDebug has extensive console logging and also shows the dotted outlines of touchZones  
pfodWeb.html and pfodWebDebug.html prompt for the connection to use and then requests the pfod menu/dwg and displays it.  

**pfodWeb.html?targetIP**  pre-selects http as the connection type.   
**pfodWeb.html?targetIP=**_\<ip\>_  immediately attempts to connect the the specified IP.    
**pfodWeb.html?serial**  pre-selectes Serial as the connection type.    
**pfodWeb.html?serial=**_\<baudrate\>_  pre-selectes Serial with the specified baudrate as the connection type.    
**pfodWeb.html?ble**  pre-selectes BLE as the connection type.    

The subdirectory pfodWeb_src contains the individual files.  **build.bat** / **build.sh** generates the combined files.

The individual files making up these webpages can also be served from the pfodDevice itself if there is a file system with sufficient space less than 200kB.  
The subdirectory data contains the .gz files to be loaded into the microprocessor's file system. 

**build_data.bat** / **build_data.sh** in pfodWeb_src generate the .gz files in the data subdirectory  
**data_index.html** -> **index.gz** in that case to be served by the microprocessor.  

When pfodWeb.html or pfodWebDebug.html is served from the pfodDevice (microprocessor) they are redirected to pfodWeb.html?target=<pfodDeviceIP> and pfodWebDebug.html?target=<pfodDeviceIP>  
to automatically connect to that pfodDevice to serve the pfod messages.  

You can use the pfodWeb files served from your microprocessor to connect to another pfodDevice via http by requesting  
**http://**_\<pfodDeviceIP\>_**/pfodWeb.html?targetIP**
to display the connection dialog from which you can select which device to connect to.  
Note: Browser security restrictions prevents connecting to Serial or BLE when pfodWeb served from **http://**_\<pfodDeviceIP\>_**  need to be served as https.       

# How-To
See [pfodWeb Installation and Tutorials](https://www.forward.com.au/pfod/pfodWeb/index.html)  

# Security and Privacy
pfodWeb does not use any third party code and does not require any internet access so you can run on a completely isolated network.  
Note: The build.bat / build.sh need nodejs to be installed and build_data.bat and build_data.sh need 7z and gzip, respectively, to be installed  

# Software License
(c)2014-2025 Forward Computing and Control Pty. Ltd.  
NSW Australia, www.forward.com.au  
This code is not warranted to be fit for any purpose. You may only use it at your own risk.  
This code may be freely used for both private and commercial use  
Provide this copyright is maintained.  

# Revisions
Version 2.0.0 removed nodejs server from pfodWebServer and bundled all files in single html files. Add Serial and BLE support    
Version 1.1.5 used gzip files to reduce microprocessor file storage requirements  
Version 1.1.3 drawing updates as response received and included dependent node packages and removed package install script from batch files  
Version 1.1.2 fixed hiding of touchActionInput labels  
Version 1.1.1 fixed loss of idx on edit  
Version 1.1.0 fix hide/unhide and other general improvements  
Version 1.0.2 fix for drag touchActions  
Version 1.0.1 fix for debug display and mainmenu updates  
Version 1.0.0 initial release  

