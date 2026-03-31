// =============================================
// SERVER MONITORING SENSOR
// Menerima data dari ESP32 via HTTP POST,
// menyimpan di memori, dan melayani website publik
// =============================================
// Cara deploy gratis: https://railway.app
// 1. Upload folder ini ke GitHub
// 2. Connect repo ke Railway
// 3. Dapatkan URL publik dari Railway
// 4. Masukkan URL ke kode ESP32
// =============================================

const express = require('express');
const cors    = require('cors');
const path    = require('path');

const app  = express();
const PORT = process.env.PORT || 3000;

// Middleware
app.use(cors());                          // Izinkan akses dari semua domain
app.use(express.json());                  // Parse JSON dari body request
app.use(express.static('public'));        // Sajikan folder public (website)

// =============================================
// PENYIMPANAN DATA (di memori)
// Catatan: Data hilang saat server restart.
// Untuk data permanen, gunakan database seperti MongoDB Atlas (gratis).
// =============================================
let dataSensor = [];
const MAX_DATA = 500; // Maksimal data yang disimpan

// Pengaturan yang bisa diubah dari website
let pengaturan = {
  interval: 5,    // Interval pengiriman (detik)
  jalan:    true  // Status monitoring
};

// =============================================
// ENDPOINT 1: Terima data dari ESP32 (POST)
// Dipanggil oleh ESP32 setiap interval
// =============================================
app.post('/api/data', (req, res) => {
  const data = req.body;

  // Validasi data minimal
  if (!data.suhu && data.suhu !== 0) {
    return res.status(400).json({ error: 'Data tidak valid' });
  }

  // Tambahkan timestamp server jika ESP32 tidak kirim waktu
  if (!data.waktu) {
    const sekarang = new Date();
    data.waktu = sekarang.toLocaleString('id-ID', { timeZone: 'Asia/Jakarta' });
  }

  // Simpan data, hapus yang paling lama jika sudah penuh
  dataSensor.push(data);
  if (dataSensor.length > MAX_DATA) {
    dataSensor.shift();
  }

  console.log(`[${new Date().toLocaleTimeString('id-ID')}] Data diterima:`, data);
  res.json({ sukses: true, jumlahData: dataSensor.length });
});

// =============================================
// ENDPOINT 2: Ambil semua data (GET)
// Digunakan oleh website untuk tabel dan grafik
// =============================================
app.get('/api/data', (req, res) => {
  res.json(dataSensor);
});

// =============================================
// ENDPOINT 3: Ambil data terbaru saja (GET)
// =============================================
app.get('/api/terbaru', (req, res) => {
  if (dataSensor.length === 0) {
    return res.json({ pesan: 'Belum ada data' });
  }
  res.json(dataSensor[dataSensor.length - 1]);
});

// =============================================
// ENDPOINT 4: Ambil pengaturan saat ini (GET)
// Dipanggil ESP32 untuk cek perintah start/stop/interval
// =============================================
app.get('/api/perintah', (req, res) => {
  res.json(pengaturan);
});

// =============================================
// ENDPOINT 5: Update pengaturan dari website (POST)
// =============================================
app.post('/api/pengaturan', (req, res) => {
  const { interval, jalan } = req.body;

  if (interval !== undefined) {
    const iv = parseInt(interval);
    if (iv >= 1 && iv <= 3600) {
      pengaturan.interval = iv;
    }
  }

  if (jalan !== undefined) {
    pengaturan.jalan = Boolean(jalan);
  }

  console.log('[Pengaturan] Diperbarui:', pengaturan);
  res.json({ sukses: true, pengaturan });
});

// =============================================
// ENDPOINT 6: Export data ke CSV (GET)
// Pengguna klik tombol di website -> unduh file CSV
// =============================================
app.get('/api/export', (req, res) => {
  if (dataSensor.length === 0) {
    return res.status(404).send('Tidak ada data untuk diekspor');
  }

  let csv = 'Waktu,Suhu(C),Kelembapan(%),Gas(ADC),Status\n';
  dataSensor.forEach(d => {
    csv += `"${d.waktu}",${d.suhu},${d.kelembapan},${d.gas},"${d.status}"\n`;
  });

  const namaFile = `data-sensor-${Date.now()}.csv`;
  res.setHeader('Content-Type', 'text/csv; charset=utf-8');
  res.setHeader('Content-Disposition', `attachment; filename="${namaFile}"`);
  res.send('\ufeff' + csv); // BOM agar Excel baca UTF-8 dengan benar
});

// =============================================
// ENDPOINT 7: Hapus semua data (DELETE)
// =============================================
app.delete('/api/data', (req, res) => {
  const jumlahDihapus = dataSensor.length;
  dataSensor = [];
  console.log(`[Data] ${jumlahDihapus} data dihapus.`);
  res.json({ sukses: true, pesanHapus: `${jumlahDihapus} data berhasil dihapus` });
});

// =============================================
// Jalankan server
// =============================================
app.listen(PORT, () => {
  console.log(`====================================`);
  console.log(` Server Monitoring Sensor`);
  console.log(` Berjalan di port: ${PORT}`);
  console.log(` Buka: http://localhost:${PORT}`);
  console.log(`====================================`);
});
