package com.example.projektzespolowy2025

import android.os.Bundle
import androidx.activity.enableEdgeToEdge
import androidx.appcompat.app.AppCompatActivity
import androidx.core.view.ViewCompat
import androidx.core.view.WindowInsetsCompat

import android.widget.Button
import android.widget.TextView
import java.net.HttpURLConnection
import java.net.URL

import java.security.SecureRandom
import java.security.cert.X509Certificate
import javax.net.ssl.*
import android.widget.EditText
import android.view.View



class MainActivity : AppCompatActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        setContentView(R.layout.activity_main)



        val db = AppDatabase.getDatabase(this)
        val dao = db.statusDao()
        val statusText = findViewById<TextView>(R.id.statusText)
        val btnOn = findViewById<Button>(R.id.btnOn)
        val btnOff = findViewById<Button>(R.id.btnOff)
        val btnRefresh = findViewById<Button>(R.id.btnRefresh)
        val sleepTimeText = findViewById<TextView>(R.id.sleepTimeText)
        val btnSetTime = findViewById<Button>(R.id.btnSetTime)
        val timeInput = findViewById<EditText>(R.id.timeInput)



        Thread {
            val result = getStatus()
            runOnUiThread { statusText.text = "Odczytano: $result" }
        }.start()

        btnOn.setOnClickListener {
            Thread {
                sendStatus("ON")
                dao.insert(StatusEntry(status = "ON", timestamp = System.currentTimeMillis()))
                val result = getStatus()
                runOnUiThread { statusText.text = "Odczytano: $result" }
                //runOnUiThread { statusText.text = "Wysłano: ON" }
            }.start()
        }

        btnOff.setOnClickListener {
            Thread {
                sendStatus("OFF")
                dao.insert(StatusEntry(status = "OFF", timestamp = System.currentTimeMillis()))
                val result = getStatus()
                runOnUiThread { statusText.text = "Odczytano: $result" }
                //runOnUiThread { statusText.text = "Wysłano: OFF" }
            }.start()
        }

        btnRefresh.setOnClickListener {
            Thread {
                val result = getStatus()
                val sleepTime = getSleepTime()
                runOnUiThread {
                    statusText.text = "Odczytano: $result"
                    sleepTimeText.text = "Czas sleep: $sleepTime s"
                }
            }.start()
        }

        btnSetTime.setOnClickListener {
            val time = timeInput.text.toString().toIntOrNull()
            if (time != null && time > 0 && time <= 604800) {
                Thread {
                    try {
                        val url = URL("https://panamint.kcir.pwr.edu.pl/~mlenczuk/Projekt/update_sleep.php?time=$time")
                        val conn = url.openConnection() as HttpURLConnection
                        conn.requestMethod = "GET"
                        conn.connect()
                        conn.inputStream.close()
                    } catch (e: Exception) {
                        e.printStackTrace()
                    }
                }.start()
            }
        }


        Thread {
            val sleepTime = getSleepTime()
            runOnUiThread {
                sleepTimeText.text = "Czas sleep: $sleepTime s"
            }
        }.start()







        ViewCompat.setOnApplyWindowInsetsListener(findViewById(R.id.main)) { v, insets ->
            val systemBars = insets.getInsets(WindowInsetsCompat.Type.systemBars())
            v.setPadding(systemBars.left, systemBars.top, systemBars.right, systemBars.bottom)
            insets
        }




        val btnShowHistory = findViewById<Button>(R.id.btnShowHistory)
        val historyText = findViewById<TextView>(R.id.historyText)


        var historyVisible = false

        btnShowHistory.setOnClickListener {
            if (!historyVisible) {
                Thread {
                    val entries = dao.getAll().take(10)
                    val formatted = entries.joinToString("\n") {
                        val time = java.text.SimpleDateFormat("HH:mm:ss").format(java.util.Date(it.timestamp))
                        "$time → ${it.status}"
                    }

                    runOnUiThread {
                        historyText.text = if (entries.isEmpty()) "Brak danych" else formatted
                        btnShowHistory.text = "Ukryj historię"
                        historyVisible = true
                    }
                }.start()
            } else {
                historyText.text = ""
                btnShowHistory.text = "Pokaż historię"
                historyVisible = false
            }
        }

        val btnClearHistory = findViewById<Button>(R.id.btnClearHistory)

        btnClearHistory.setOnClickListener {
            Thread {
                dao.clear()
                runOnUiThread {
                    historyText.text = ""
                    btnShowHistory.text = "Pokaż historię"
                    historyVisible = false
                }
            }.start()
        }




    }


    private fun sendStatus(value: String) {
        try {
            val url = URL("https://panamint.kcir.pwr.edu.pl/~mlenczuk/Projekt/update_status.php?status=$value")
            val connection = url.openConnection() as HttpURLConnection
            connection.requestMethod = "GET"
            connection.connect()
            connection.inputStream.close()
        } catch (e: Exception) {
            e.printStackTrace()
        }
    }


    private fun getStatus(): String {
        return try {
            val url = URL("https://panamint.kcir.pwr.edu.pl/~mlenczuk/Projekt/status.txt")
            val trustAllCerts = arrayOf<TrustManager>(object : X509TrustManager {
                override fun checkClientTrusted(chain: Array<out X509Certificate>?, authType: String?) {}
                override fun checkServerTrusted(chain: Array<out X509Certificate>?, authType: String?) {}
                override fun getAcceptedIssuers(): Array<X509Certificate> = arrayOf()
            })

            val sc = SSLContext.getInstance("SSL")
            sc.init(null, trustAllCerts, SecureRandom())
            HttpsURLConnection.setDefaultSSLSocketFactory(sc.socketFactory)
            HttpsURLConnection.setDefaultHostnameVerifier { _, _ -> true }

            val connection = url.openConnection() as HttpsURLConnection
            connection.connect()
            val text = connection.inputStream.bufferedReader().readText().trim()
            connection.disconnect()
            text
        } catch (e: Exception) {
            e.printStackTrace()
            "Błąd połączenia"
        }
    }

    private fun getSleepTime(): String {
        return try {
            val url = URL("https://panamint.kcir.pwr.edu.pl/~mlenczuk/Projekt/sleep_time.txt")

            val trustAllCerts = arrayOf<TrustManager>(object : X509TrustManager {
                override fun checkClientTrusted(chain: Array<out X509Certificate>?, authType: String?) {}
                override fun checkServerTrusted(chain: Array<out X509Certificate>?, authType: String?) {}
                override fun getAcceptedIssuers(): Array<X509Certificate> = arrayOf()
            })

            val sc = SSLContext.getInstance("SSL")
            sc.init(null, trustAllCerts, SecureRandom())
            HttpsURLConnection.setDefaultSSLSocketFactory(sc.socketFactory)
            HttpsURLConnection.setDefaultHostnameVerifier { _, _ -> true }

            val connection = url.openConnection() as HttpsURLConnection
            connection.connect()
            val text = connection.inputStream.bufferedReader().readText().trim()
            connection.disconnect()
            text
        } catch (e: Exception) {
            e.printStackTrace()
            "Błąd odczytu czasu"
        }
    }



}