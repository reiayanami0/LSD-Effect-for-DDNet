
 # 🍃 DDNet / Teeworlds LSD Effect

 **A simple LSD screen effect component with hue-cycling and screen wobble for DDNet clients.**
 <img src="https://media.tenor.com/Ylqui-QhuO0AAAAC/pikachu-drool.gif" width="1000">

<div align="center">
  <a href="https://github.com/reiayanami0/LSD-Effect-for-DDNet">
    <img src="https://img.shields.io/badge/For-Teeworlds-98d243?style=for-the-badge" alt="For Teeworlds">
  </a>
  
  <br/>

  <img src="https://img.shields.io/badge/C++-00599C?style=flat-square&logo=c%2B%2B&logoColor=white" alt="C++">
  <img src="https://img.shields.io/badge/Compatible%20with-DDNet-lightgrey?style=flat-square" alt="Compatible with DDNet">
</div>

# Implemention
## 1. Drop the folder in

Copy this whole folder to:

    src/game/client/components/lsd/

## 2. Root CMakeLists.txt

Add, near where other subdirectories/components are configured:

```cmake
components/lsd/lsd_effect.cpp
components/lsd/lsd_effect.h
```

## 3. game/client/gameclient.h 

```cpp
#include "components/lsd/lsd_effect.h"
```
somewhere in IGameClient public
```cpp
CLsdEffect m_LsdEffect;
```

## 4. game/client/gameclient.cpp

```cpp
m_vpAll.push_back(&m_LsdEffect); // end of wherever &m_Camera etc. are pushed
```


## 5. game/client/components/camera.cpp 

Find where the camera's current zoom value is turned into the actual 
call for the world (search for `m_Zoom` near a
`MapScreen` call in `CCamera::OnRender`).

```cpp
m_Zoom *= GameClient()->m_LsdEffect.ZoomModifier();
```
In 19.8, just below 
```cpp
m_WasSpectating = GameClient()->m_Snap.m_SpecInfo.m_Active;`
```

```cpp
float CCamera::EffectiveZoom() const
{
    return m_Zoom * GameClient()->m_LsdEffect.ZoomModifier();
}
```

## 6. game/client/components/camera.h

```cpp
	float EffectiveZoom() const;
```

## 7. Build and test
```bash
cmake -Bbuild -GNinja && cmake --build build
```

```
lsd_toggle          # turn the effect on/off
lsd_speed 1.5        # faster hue cycling
lsd_intensity 0.25   # stronger tint
lsd_wobble 0.12       # bigger zoom breathing
```

Bind a key for convenience: `bind l lsd_toggle`
