# Roabmap for **Tangent UE5 Plugin** ammendments and improvements

*This document will serve as a roadmap of issues and imrpovements that are to be made to the Tangnet plugin for Unreal Engine 5.*

##June 2026 Roadmap

1. Identification and fix of crash causes when loading a new level without saving multiple changes made with Tangent Wave 2 desk.
   
2. Identification of problem causing "Freeze" nDisplay viewports to not transact over multi-user when trained to a button on the panel.
     *Note other nDisplay actions do work over mutli-user when trained to buttons on panel.*
  
3. Identify and fix the sublevel selection issue caused by the use of "Object Name" over "Display Name" / "Actor Label".
     *The problem occurs when you have objects across multiple sub-levels in editor, each sub-level starts with objects having a name at 0 (such as "StaticMesh_0") this means that if you have 3 sub-levels you will have 3 "StaticMesh_0"'s. The Tangent panel cannot identify each individually due to all having the same object name. The solution is to change from using the object name to the "Display Name" / actor label. This will solve the issue with the caviat being all objects must be named uniquely within the editor.*

4. Identify how OSC can be utilised in Tangent Mapper for UE
     *Currently the mapper allows text strings to be sent but doesn't give the ability to set the value of set string. It appears as if it will always be 2 consecutive values of 0/1 or On/Off. If OSC command is set on a rotary then it does allow for values to be sent however, but not as string, only Int, Float, etc.*
