package com.example.projektzespolowy2025

import androidx.room.Dao
import androidx.room.Insert
import androidx.room.Query

@Dao
interface StatusDao {
    @Insert
    fun insert(entry: StatusEntry)

    @Query("SELECT * FROM status_history ORDER BY timestamp DESC")
    fun getAll(): List<StatusEntry>

    @Query("DELETE FROM status_history")
    fun clear()
}
