# Truck Navigation Message

geometry_msgs/Pose[] poses
float64[] timestamps
float64 confidence
---
# Service definitions
---
# StartNavigation.srv
geometry_msgs/PoseStamped goal
---
bool success
string message
---
# StopNavigation.srv
---
bool success
string message
---
# PauseNavigation.srv
bool pause
---
bool success
string message