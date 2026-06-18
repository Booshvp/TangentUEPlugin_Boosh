# Roabmap for **Tangent UE5 Plugin** ammendments and improvements

*This document will serve as a roadmap of issues and improvements that are to be made to the Tangnet plugin for Unreal Engine 5.*

## June 2026 Roadmap



1. Identification and fixing of crash causes when loading a new level without saving multiple changes made with Tangent Wave 2 desk.
     *This seems to have been caused when moving the camera at some point and then choosing not to save when loading a new level.*

   // Tested and solution is pushed on 18th June 2026//

    *This looks like the issue was a result of the plugin saving a reference to the last selected camera. This has been removed, and the crashing has stopped*


   
3. Identification of the problem causing "Freeze" in the nDisplay viewports to not transact over multi-user when trained to a button on the panel.
     *Note other nDisplay actions do work over mutli-user when trained to buttons on panel.*

   // Tested and is working as of 18th June 2026 // 


  
4. Identify and fix the sublevel selection issue caused by the use of "Object Name" over "Display Name" / "Actor Label".
     *The problem occurs when you have objects across multiple sub-levels in the editor. Each sub-level starts with objects having a name at 0 (such as "StaticMesh_0"). This means that if you have 3 sub-levels you will have 3 "StaticMesh_0"'s. The Tangent panel cannot identify each individually due to all having the same object name. The solution is to change from using the object name to the "Display Name" / actor label. This will solve the issue with the caveat being that all objects must be named uniquely within the editor.*



5. Identify how OSC can be utilised in Tangent Mapper for UE
     *Currently, the mapper allows text strings to be sent but doesn't give the ability to set the value of "set string". It appears as if it will always be 2 consecutive values of 0/1 or On/Off. If OSC command is set on a rotary, then it does allow for values to be sent, however, not as string, only Int, Float, etc.*
