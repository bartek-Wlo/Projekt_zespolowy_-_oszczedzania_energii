package com.example.projektzespolowy2025

import androidx.room.Entity
import androidx.room.PrimaryKey

@Entity(tableName = "status_history")
data class StatusEntry(
    @PrimaryKey(autoGenerate = true) val id: Int = 0,
    val status: String,
    val timestamp: Long
)

