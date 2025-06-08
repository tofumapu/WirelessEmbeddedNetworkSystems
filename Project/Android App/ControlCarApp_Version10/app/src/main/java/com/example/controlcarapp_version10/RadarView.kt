package com.example.controlcarapp_version10
import android.content.Context
import android.graphics.Canvas
import android.graphics.Color
import android.graphics.Paint
import android.graphics.PointF
import android.util.AttributeSet
import android.view.View
import java.util.LinkedList
import kotlin.math.cos
import kotlin.math.min
import kotlin.math.sin


class RadarView @JvmOverloads constructor(
    context: Context, attrs: AttributeSet? = null, defStyleAttr: Int = 0
) : View(context, attrs, defStyleAttr) {

    private val radarPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        style = Paint.Style.STROKE
        color = Color.GREEN
        strokeWidth = 2f
    }

    private val sweepPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        style = Paint.Style.FILL_AND_STROKE
        color = Color.argb(80, 0, 255, 0)
    }

    private val obstaclePaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        style = Paint.Style.FILL
        color = Color.RED
    }

    private var centerX: Float = 0f
    private var centerY: Float = 0f
    private var radius: Float = 0f
    private var sweepAngleDegrees: Float = 0f

    private val obstacles = LinkedList<ObstaclePoint>()
    private val MAX_OBSTACLES = 100
    private val OBSTACLE_POINT_DURATION = 5000L

    private val MAX_DISTANCE_METERS = 2.0f

    data class ObstaclePoint(
        var angleRad: Float,
        var distancePx: Float,
        val timestamp: Long = System.currentTimeMillis()
    )

    override fun onSizeChanged(w: Int, h: Int, oldw: Int, oldh: Int) {
        super.onSizeChanged(w, h, oldw, oldh)
        centerX = w / 2f
        centerY = h / 2f
        radius = min(w, h) / 2f * 0.9f
    }

    override fun onDraw(canvas: Canvas) {
        super.onDraw(canvas)

        drawRadarBackground(canvas)
        drawSweep(canvas)
        drawObstacles(canvas)
    }

    private fun drawRadarBackground(canvas: Canvas) {
        val circles = 4
        for (i in 1..circles) {
            canvas.drawCircle(centerX, centerY, radius * i / circles, radarPaint)
        }

        val lines = 8
        for (i in 0 until lines) {
            val angleRad = Math.toRadians((i * 360.0 / lines)).toFloat()
            val endX = centerX + radius * cos(angleRad)
            val endY = centerY + radius * sin(angleRad)
            canvas.drawLine(centerX, centerY, endX, endY, radarPaint)
        }
    }

    private fun drawSweep(canvas: Canvas) {
        val sweepAngleRad = Math.toRadians(sweepAngleDegrees.toDouble()).toFloat()
        val sweepEndX = centerX + radius * cos(sweepAngleRad)
        val sweepEndY = centerY + radius * sin(sweepAngleRad)
        sweepPaint.strokeWidth = 4f // Độ dày của đường quét
        sweepPaint.color = Color.argb(100, 0, 255, 0)
        canvas.drawLine(centerX, centerY, sweepEndX, sweepEndY, sweepPaint)
    }

    private fun drawObstacles(canvas: Canvas) {
        val currentTime = System.currentTimeMillis()
        val iterator = obstacles.iterator()
        while (iterator.hasNext()) {
            val obstacle = iterator.next()
            if (currentTime - obstacle.timestamp > OBSTACLE_POINT_DURATION) {
                iterator.remove()
                continue
            }

            val obstacleX = centerX + obstacle.distancePx * cos(obstacle.angleRad)
            val obstacleY = centerY + obstacle.distancePx * sin(obstacle.angleRad)
            val obstacleRadius = 8f
            canvas.drawCircle(obstacleX, obstacleY, obstacleRadius, obstaclePaint)
        }
    }
    fun updateSweepAngle(angleDegrees: Float) {
        this.sweepAngleDegrees = angleDegrees % 360
        invalidate() // Yêu cầu vẽ lại View
    }

    fun addObstacle(angleDegrees: Float, distanceMeters: Float) {
        if (distanceMeters <= 0 || distanceMeters > MAX_DISTANCE_METERS) {
            return
        }

        val angleRad = Math.toRadians(angleDegrees.toDouble() - 90.0).toFloat()
        val distancePx = (distanceMeters / MAX_DISTANCE_METERS) * radius

        if (obstacles.size >= MAX_OBSTACLES) {
            obstacles.poll()
        }
        obstacles.add(ObstaclePoint(angleRad, distancePx))
        invalidate()
    }
    fun clearObstacles() {
        obstacles.clear()
        invalidate()
    }
}