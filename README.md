# Sistem Early Warning Banjir - Dua Desa

Proyek ini adalah sistem peringatan dini banjir berbasis IoT untuk dua desa menggunakan komunikasi MQTT dan visualisasi data secara real-time melalui dashboard berbasis Node.js. Sistem ini terdiri dari dua mikrokontroler yang ditempatkan di Desa 1 dan Desa 2, serta dashboard untuk pemerintah, kepala desa, dan warga.


## Alur Sistem

1. Desa 2 mengukur tinggi air dan **mengirim data melalui esp-now ke esp desa 1"
2. Desa 1 bertindak sebagai node pusat yang menerima data dari Desa 2, lalu menggabungkan dengan datanya sendiri dan mengirimkan topic nya ke mqtt broker.
3. Server Node.js:
   - Meng-subscribe topik MQTT dari kedua desa.
   - Menyimpan dan menyajikan data ke client via Via TCP (tanpa SSL/TLS).
4. Dashboard Web:
   - Pemerintah: Melihat grafik dan status hujan real-time dari kedua desa.
   - Kepala Desa 1 & 2: Melihat data real-time khusus desanya.
   - Warga: Melihat data update tiap 10 detik saja (bukan real-time).

## Teknologi yang Digunakan

- * ESP32 C++ * untuk perangkat di lapangan (desa1.cpp, desa2.cpp).
- * MQTT * via [Maqiatto](https://maqiatto.com/) sebagai broker.
- * Node.js * untuk server backend (`server.js`).
- * HTML + JavaScript (frontend) * untuk dashboard pengguna.
- * Chart.js * untuk visualisasi grafik.

## Cara Menjalankan

1. * Pasang perangkat Desa 1 dan Desa 2 * dengan firmware dari `program arduinonya/`.
2. * Jalankan server: *

   ```bash
   npm install
   node server.js

