The 2 files are used to configure the Grovestreams example.

Grovestreams_Blueprint.txt is used by the user, just after he created his Grovestreams account, when he was asked to create the "Orga"
To create the orga the user can use the option: "Advanced / Create with a custom blueprint" and select the Grovestreams_Blueprint.txt file.
So all the component and dashboard will be created automatically and in line with the Grovestreams FW example.

Grovestreams_Setup.txt is used by the user at boot, in boot menu, in "Grovestreams" option.
The user can (and must the first time) configure the generic FW with his Grovestreams credentials.
For this he can enter manually all the needed parameters or use Grovestreams_Setup.txt file (more friendly and no type error possible).
After selecting "Grovestreams" in boot menu, select "File", "Send File ..." and select Grovestreams_Setup.txt.
Every non commented out line in the txt file is an input for the FW.

Note Grovestreams_Blueprint.txt does not need modification (use as it is provided) but Grovestreams_Setup.txt needs to be updated, at least for API Keys.
Indeed the user must copy API Keys from his Grovestreams/Orga account and paste them (over default API Keys) into the Grovestreams_Setup.txt before using it.


