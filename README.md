# kraky-v2
Kraky2 is the second version of the DIY and open-hardware poject inspired by the Flipper Zero.
It uses the esp32-S3 microntroller to manage all the different modules like the NFC,RFID, and some.

The module is divided in 2 layers, the front one houses the esp32, Wifi and Sub-GHz antennas, buttons, IR and a part of the
power delivery system; the back one houses the NFC and RFID modules on top of the rest of the power delivery system.

Using the four buttons you can take advantage of all the modules that composes the kraky2, you can also use it connected to 
a battery or via the usb-c port that is integrated in the circuit.

I would suggest putting first the esp32 as is the most difficult part to put, then the others chip, all the capacitors, resistors, inductors and so on and for last all the
tht components because they are the ones that pass through the board so they are more simple to put.

I started this journey because i want to create a better version of my project kraky, and i'm doing it by first using smd components instead of the 
tht modules so i can do some expirience soldering very small parts and stopping using pre made modules but recreating them myself.

---

# Main Components
| Function | Component |
|--------|----------|
| Logic board | **ESP32-S3 N16R8** |
| wifi | **nrf24l01** |
| Sub-GHz | **CC1101** |
| RFID | **MFRC522** |
| NFC | **PN532** |
| Battery charger | **CN3163** |
| Battery | Li-ion |
| IR Receiver | **TSOP38238** |
| IR Transmitter | IR LED |
| microSD Slot | **MSD4A** |
| Buttons | 4 tactile buttons |
| Display | TFT Display |

---

# WIRING DIAGRAM

# ESP32

<img width="605" height="549" alt="Screenshot 2026-05-21 200839" src="https://github.com/user-attachments/assets/a926d4ba-9eac-44f2-b257-eeea607c0d92" />

 # POWER DELIVERY SYSTEM
 
<img width="1201" height="713" alt="Screenshot 2026-05-31 180216" src="https://github.com/user-attachments/assets/ca888b68-b830-4657-a2c7-ea668f36ed33" />


# IR

<img width="855" height="608" alt="Screenshot 2026-05-21 200859" src="https://github.com/user-attachments/assets/95f98821-4e4d-4ce2-911e-a27a34e1521e" />

# WIFI

<img width="617" height="313" alt="Screenshot 2026-05-21 200908" src="https://github.com/user-attachments/assets/7c61bb99-3f7c-4587-89f7-4b293f64252c" />

# SUB-GHZ

<img width="922" height="375" alt="Screenshot 2026-05-21 200920" src="https://github.com/user-attachments/assets/6ed7cd88-4e8f-46d6-b89a-9477e469bf03" />

# MICROSD SLOT

<img width="274" height="292" alt="Screenshot 2026-05-21 200913" src="https://github.com/user-attachments/assets/a83eefe9-5326-40f6-8fb7-e8ce4b47d891" />

# NFC

<img width="1077" height="692" alt="Screenshot 2026-05-21 200932" src="https://github.com/user-attachments/assets/4bd944f0-6582-42e5-bcc8-0ec30ba9659c" />

# RFID

<img width="676" height="367" alt="Screenshot 2026-05-21 200938" src="https://github.com/user-attachments/assets/0d7734a0-ea96-4494-ac09-19b77f814a59" />

# BUTTONS

<img width="941" height="490" alt="Screenshot 2026-05-21 200945" src="https://github.com/user-attachments/assets/ec05a574-0657-409c-883f-e03cc27f2e70" />

# DISPLAY

<img width="340" height="215" alt="Screenshot 2026-05-21 200952" src="https://github.com/user-attachments/assets/dc62b30f-9696-4c1a-b3b3-27470e7d3c68" />

---

# PCB

<img width="722" height="697" alt="Screenshot 2026-05-21 210101" src="https://github.com/user-attachments/assets/075f0950-81c8-4a14-974e-6da5919a4297" />


---

# 3D RENDER

<img width="761" height="826" alt="Screenshot 2026-06-04 145629" src="https://github.com/user-attachments/assets/b731df5f-cc39-4309-a4c6-40bbe3e8c465" />

<img width="680" height="805" alt="Screenshot 2026-06-04 145643" src="https://github.com/user-attachments/assets/2943e095-c932-45d9-86d4-1423ae9d60f9" />




---
# Responsible Use
Kraky2 is built to make it easier for anyone to explore and understand how multifunction electronic devices work in everyday life. It’s not intended for bypassing security systems or breaking any laws.

All features are implemented with an educational and experimental focus.

---

# A presentation page

<img width="1414" height="2000" alt="kraky2 poster (3)" src="https://github.com/user-attachments/assets/ae7f23db-7d71-487b-a487-06ccc0b4d5c1" />



---

# Tools Used

- KiCad 10
- Fusion360
- PlatformIO
---
## Contributing

Kraky is an **open and educational project**.  
Feedback, suggestions, and improvements are always welcome.

---
