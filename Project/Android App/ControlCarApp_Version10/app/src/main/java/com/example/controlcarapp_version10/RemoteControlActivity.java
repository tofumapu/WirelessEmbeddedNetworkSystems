package com.example.controlcarapp_version10;

import android.Manifest;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.speech.RecognizerIntent;
import android.util.Log;
import android.widget.Button;
import android.widget.ImageButton;
import android.widget.RadioButton;
import android.widget.Switch;
import android.widget.TextView;
import android.widget.Toast;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;
import com.google.firebase.database.DataSnapshot;
import com.google.firebase.database.DatabaseError;
import com.google.firebase.database.DatabaseReference;
import com.google.firebase.database.FirebaseDatabase;
import com.google.firebase.database.ValueEventListener;
import java.util.ArrayList;
import java.util.Locale;


public class RemoteControlActivity extends AppCompatActivity {
    private RadarView radarView;
    private android.os.Handler sweepHandler;
    private float currentSweepAngle = 0f;
    private static final int SWEEP_INTERVAL_MS = 50;
    private static final float SWEEP_INCREMENT_DEG = 5f;
    private DatabaseReference RootRef;

    private Button Left1, Left2, Left3;
    private Button Forward, Stop, Backward;
    private Button Right1, Right2, Right3;
    private ImageButton microButton;

    private TextView textViewCurrentState;
    private TextView textViewRecognizedCommand;
    private TextView textViewNotification;
    private TextView textViewObstacleDistance;
    private RadioButton radioButtonObstacleDetected;
    private Switch switchObstacleAvoidance;
    private Switch switchFollowObject;
    private ControlState currentCarState = ControlState.IDLE;

    private static final String TAG = "RemoteControlActivity";
    private static final int REQUEST_CODE_SPEECH_INPUT = 100;
    private static final int REQUEST_CODE_AUDIO_PERMISSION = 101;

    @Override
    protected void onCreate(Bundle savedInstanceState) {

        Left1 = findViewById(R.id.left1);
        Left2 = findViewById(R.id.left2);
        Left3 = findViewById(R.id.left3);

        Forward = findViewById(R.id.forward);
        Stop = findViewById(R.id.Stop);
        Backward = findViewById(R.id.backward);

        Right1 = findViewById(R.id.Right1);
        Right2 = findViewById(R.id.Right2);
        Right3 = findViewById(R.id.Right3);

        super.onCreate(savedInstanceState);

        setContentView(R.layout.activity_remote_control);

        radarView = findViewById(R.id.radarView);
        sweepHandler = new android.os.Handler(getMainLooper());
        startRadarSweep();
        RootRef = FirebaseDatabase.getInstance().getReference();



        microButton = findViewById(R.id.imageButton2);
        textViewCurrentState = findViewById(R.id.textView2);
        textViewRecognizedCommand = findViewById(R.id.textView7);
        textViewNotification = findViewById(R.id.radioButton);
        textViewObstacleDistance = findViewById(R.id.textView4);
        radioButtonObstacleDetected = findViewById(R.id.radioButton);
        switchFollowObject = findViewById(R.id.switch2);
        switchObstacleAvoidance = findViewById(R.id.switch1);
        updateStateDisplay(ControlState.IDLE, "IDLE (Khởi tạo)");
        textViewRecognizedCommand.setText("...");
        Left1.setOnClickListener(v -> {
            RootRef.child("Move").child("move").setValue("5");
            updateStateDisplay(ControlState.TURNING_LEFT_1, "Rẽ Trái 1 (L1)");
        });
        Left2.setOnClickListener(v -> {
            RootRef.child("Move").child("move").setValue("6");
            updateStateDisplay(ControlState.TURNING_LEFT_2, "Rẽ Trái 2 (L2)");
        });
        Left3.setOnClickListener(v -> {
            RootRef.child("Move").child("move").setValue("7");
            updateStateDisplay(ControlState.TURNING_LEFT_3, "Rẽ Trái 3 (L3)");
        });

        Right1.setOnClickListener(v -> {
            RootRef.child("Move").child("move").setValue("2");
            updateStateDisplay(ControlState.TURNING_RIGHT_1, "Rẽ Phải 1 (R1)");
        });
        Right2.setOnClickListener(v -> {
            RootRef.child("Move").child("move").setValue("3");
            updateStateDisplay(ControlState.TURNING_RIGHT_2, "Rẽ Phải 2 (R2)");
        });
        Right3.setOnClickListener(v -> {
            RootRef.child("Move").child("move").setValue("4");
            updateStateDisplay(ControlState.TURNING_RIGHT_3, "Rẽ Phải 3 (R3)");
        });
        Forward.setOnClickListener(v -> {
            RootRef.child("Move").child("move").setValue("1");
            updateStateDisplay(ControlState.MOVING_FORWARD, "Tiến");
        });
        Stop.setOnClickListener(v -> {
            RootRef.child("Move").child("move").setValue("0");
            updateStateDisplay(ControlState.STOPPED, "Dừng");
        });
        Backward.setOnClickListener(v -> {
            RootRef.child("Move").child("move").setValue("8");
            updateStateDisplay(ControlState.MOVING_BACKWARD, "Lùi");
        });
        microButton.setOnClickListener(v -> {
            checkPermissionsAndStartSpeechRecognition();
        });
        switchObstacleAvoidance.setOnCheckedChangeListener((buttonView, isChecked) -> {
            RootRef.child("ControlSettings").child("ObstacleAvoidance").setValue(isChecked);
            Toast.makeText(RemoteControlActivity.this, "Tránh vật cản: " + (isChecked ? "BẬT" : "TẮT"), Toast.LENGTH_SHORT).show();
        });

        if (switchObstacleAvoidance != null) {
            switchObstacleAvoidance.setOnCheckedChangeListener((buttonView, isChecked) -> {
                RootRef.child("ControlSettings").child("ObstacleAvoidance").setValue(isChecked)
                        .addOnSuccessListener(aVoid -> Log.d(TAG, "ObstacleAvoidance state sent: " + isChecked))
                        .addOnFailureListener(e -> Log.e(TAG, "Failed to send ObstacleAvoidance state", e));
                Toast.makeText(RemoteControlActivity.this, "Tránh vật cản: " + (isChecked ? "BẬT" : "TẮT"), Toast.LENGTH_SHORT).show();
            });
        }

        if (switchFollowObject != null) {
            switchFollowObject.setOnCheckedChangeListener((buttonView, isChecked) -> {
                RootRef.child("ControlSettings").child("FollowObjectMode").setValue(isChecked)
                        .addOnSuccessListener(aVoid -> Log.d(TAG, "FollowObjectMode state sent: " + isChecked))
                        .addOnFailureListener(e -> Log.e(TAG, "Failed to send FollowObjectMode state", e));
                Toast.makeText(RemoteControlActivity.this, "Đi theo vật cản: " + (isChecked ? "BẬT" : "TẮT"), Toast.LENGTH_SHORT).show();

                if (isChecked && switchObstacleAvoidance != null) {
                    switchObstacleAvoidance.setChecked(false);
                }
            });
        }
        setupFirebaseSensorListener();
        checkPermissions();
    }
    private void updateStateDisplay(ControlState newState, String stateDescription) {
        ControlState previousState = currentCarState;
        currentCarState = newState;

        // Update the TextView for current state
        if (textViewCurrentState != null) {
            textViewCurrentState.setText(stateDescription);
        }
        Log.d(TAG, "Trạng thái thay đổi: Từ [" + previousState + "] -> Sang [" + currentCarState + "] (" + stateDescription + ")");
    }
    private void checkPermissions() {
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.RECORD_AUDIO)
                != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this,
                    new String[]{Manifest.permission.RECORD_AUDIO},
                    REQUEST_CODE_AUDIO_PERMISSION);
        }
    }
    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        switch (requestCode) {
            case REQUEST_CODE_AUDIO_PERMISSION: {
                if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                    Toast.makeText(this, "Đã cấp quyền Micro", Toast.LENGTH_SHORT).show();
                } else {
                    Toast.makeText(this, "Cần quyền Micro để sử dụng điều khiển giọng nói", Toast.LENGTH_LONG).show();
                }
                return;
            }
        }
    }
    private void checkPermissionsAndStartSpeechRecognition() {
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.RECORD_AUDIO)
                == PackageManager.PERMISSION_GRANTED) {
            startSpeechRecognition();
        } else {
            checkPermissions();
        }
    }
    private void startSpeechRecognition() {
        Intent intent = new Intent(RecognizerIntent.ACTION_RECOGNIZE_SPEECH);
        intent.putExtra(RecognizerIntent.EXTRA_LANGUAGE_MODEL, RecognizerIntent.LANGUAGE_MODEL_FREE_FORM);
        intent.putExtra(RecognizerIntent.EXTRA_LANGUAGE, Locale.getDefault());
        intent.putExtra(RecognizerIntent.EXTRA_PROMPT, "Hãy nói lệnh điều khiển...");
        intent.putExtra(RecognizerIntent.EXTRA_MAX_RESULTS, 1);
        try {
            startActivityForResult(intent, REQUEST_CODE_SPEECH_INPUT);
        } catch (Exception e) {
            Log.e(TAG, "Lỗi khi khởi động nhận diện giọng nói", e);
        }
    }
    @Override
    protected void onActivityResult(int requestCode, int resultCode, @Nullable Intent data) {
        super.onActivityResult(requestCode, resultCode, data);

        if (requestCode == REQUEST_CODE_SPEECH_INPUT) {
            if (resultCode == RESULT_OK && data != null) {
                ArrayList<String> result = data.getStringArrayListExtra(RecognizerIntent.EXTRA_RESULTS);
                if (result != null && !result.isEmpty()) {
                    String recognizedText = result.get(0);
                    textViewRecognizedCommand.setText(recognizedText);
                    processVoiceCommand(recognizedText);

                } else {
                    textViewRecognizedCommand.setText("Không nhận dạng được");
                    textViewNotification.setText("Thông báo: Vui lòng nói rõ hơn");
                }
            } else if (resultCode == RESULT_CANCELED) {
                textViewRecognizedCommand.setText("Đã hủy");
                textViewNotification.setText("Thông báo: Đã hủy nhận diện giọng nói");
            } else {
                textViewRecognizedCommand.setText("Lỗi");
                textViewNotification.setText("Thông báo: Lỗi nhận diện giọng nói");
                Log.e(TAG, "Lỗi nhận diện giọng nói, resultCode: " + resultCode);
            }
        }
    }
    private void processVoiceCommand(String command) {
        String lowerCaseCommand = command.toLowerCase();
        ControlState commandState = ControlState.IDLE;
        if (lowerCaseCommand.contains("tiến") || lowerCaseCommand.contains("go forward") || lowerCaseCommand.contains("forward")) {
            commandState = ControlState.MOVING_FORWARD;
        } else if (lowerCaseCommand.contains("lùi") || lowerCaseCommand.contains("go backward") || lowerCaseCommand.contains("backward")) {
            commandState = ControlState.MOVING_BACKWARD;
        } else if (lowerCaseCommand.contains("dừng") || lowerCaseCommand.contains("stop")) {
            commandState = ControlState.STOPPED;
        } else if (lowerCaseCommand.contains("tiến trái") || lowerCaseCommand.contains("go ahead left") || lowerCaseCommand.contains("l1")) {
            commandState = ControlState.TURNING_LEFT_1;
        } else if (lowerCaseCommand.contains("xoay trái") || lowerCaseCommand.contains("go left") || lowerCaseCommand.contains("l2")) {
            commandState = ControlState.TURNING_LEFT_2;
        } else if (lowerCaseCommand.contains("lùi trái") || lowerCaseCommand.contains("go back left") || lowerCaseCommand.contains("l3")) {
            commandState = ControlState.TURNING_LEFT_3;
        } else if (lowerCaseCommand.contains("tiến phải") || lowerCaseCommand.contains("go ahead right") || lowerCaseCommand.contains("r1")) {
            commandState = ControlState.TURNING_RIGHT_1;
        } else if (lowerCaseCommand.contains("xoay phải") || lowerCaseCommand.contains("go right") || lowerCaseCommand.contains("r2")) {
            commandState = ControlState.TURNING_RIGHT_2;
        } else if (lowerCaseCommand.contains("lùi phải") || lowerCaseCommand.contains("go back right") || lowerCaseCommand.contains("r3")) {
            commandState = ControlState.TURNING_RIGHT_3;
        }
        sendControlCommand(commandState);
    }

    private void sendControlCommand(ControlState state) {
        String firebaseCommandValue = "0";

        switch (state) {
            case MOVING_FORWARD:
                firebaseCommandValue = "1";
                break;
            case TURNING_RIGHT_1:
                firebaseCommandValue = "2";
                break;
            case TURNING_RIGHT_2:
                firebaseCommandValue = "3";
                break;
            case TURNING_RIGHT_3:
                firebaseCommandValue = "4";
                break;
            case TURNING_LEFT_1:
                firebaseCommandValue = "5";
                break;
            case TURNING_LEFT_2:
                firebaseCommandValue = "6";
                break;
            case TURNING_LEFT_3:
                firebaseCommandValue = "7";
                break;
            case MOVING_BACKWARD:
                firebaseCommandValue = "8";
                break;
            case STOPPED:
                firebaseCommandValue = "0";
                break;
            case IDLE:
                firebaseCommandValue = "0";
                break;
            default:
                firebaseCommandValue = "0";
                Log.w(TAG, "Unknown ControlState received: " + state + ". Sending STOP command.");
                break;
        }

        RootRef.child("Move").child("move").setValue(firebaseCommandValue)
                .addOnSuccessListener(aVoid -> {
                    updateStateDisplay(state, getStateDescription(state));
                    textViewNotification.setText("Thông báo: Đã gửi lệnh " + getStateDescription(state));
                })
                .addOnFailureListener(e -> {
                    textViewNotification.setText("Thông báo: Lỗi gửi lệnh " + getStateDescription(state));
                    Log.e(TAG, "Lỗi khi gửi lệnh Firebase cho trạng thái: " + state, e);
                });
    }

    private String getStateDescription(ControlState state) {
        switch (state) {
            case IDLE: return "IDLE";
            case MOVING_FORWARD: return "Tiến";
            case MOVING_BACKWARD: return "Lùi";
            case STOPPED: return "Dừng";
            case TURNING_LEFT_1: return "Rẽ Trái 1 (L1)";
            case TURNING_LEFT_2: return "Rẽ Trái 2 (L2)";
            case TURNING_LEFT_3: return "Rẽ Trái 3 (L3)";
            case TURNING_RIGHT_1: return "Rẽ Phải 1 (R1)";
            case TURNING_RIGHT_2: return "Rẽ Phải 2 (R2)";
            case TURNING_RIGHT_3: return "Rẽ Phải 3 (R3)";
            default: return state.toString();
        }
    }

    private final Runnable sweepRunnable = new Runnable() {
        @Override
        public void run() {
            currentSweepAngle = (currentSweepAngle + SWEEP_INCREMENT_DEG) % 360;
            if (radarView != null) {
                radarView.updateSweepAngle(currentSweepAngle);
            }
            sweepHandler.postDelayed(this, SWEEP_INTERVAL_MS);
        }
    };

    private void startRadarSweep() {
        sweepHandler.post(sweepRunnable);
    }

    private void stopRadarSweep() {
        sweepHandler.removeCallbacks(sweepRunnable);
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        stopRadarSweep();
    }
    private void onSensorDataReceived(float angle, float distance) {
        if (radarView != null) {
            radarView.addObstacle(angle, distance);
        }
        if (textViewObstacleDistance != null) {
            textViewObstacleDistance.setText(String.format(Locale.getDefault(), "%.1f cm (%.1f°)", distance * 100, angle));
        }
    }
    private void setupFirebaseSensorListener() {
        DatabaseReference radarDataRef = RootRef.child("RadarData");
        radarDataRef.addValueEventListener(new ValueEventListener() {
            @Override
            public void onDataChange(@NonNull DataSnapshot dataSnapshot) {
                Log.d(TAG, "RadarData onDataChange triggered. Path: " + dataSnapshot.getRef().toString());
                try {
                    if (dataSnapshot.exists()) {
                        Log.d(TAG, "RadarData snapshot exists. Data: " + dataSnapshot.getValue());
                        Object angleObj = dataSnapshot.child("angle").getValue();
                        Object distanceObj = dataSnapshot.child("distance").getValue();

                        Log.d(TAG, "Raw angle object from Firebase: " + angleObj + (angleObj != null ? " (Type: " + angleObj.getClass().getSimpleName() + ")" : ""));
                        Log.d(TAG, "Raw distance object from Firebase: " + distanceObj + (distanceObj != null ? " (Type: " + distanceObj.getClass().getSimpleName() + ")" : ""));

                        Float angle = dataSnapshot.child("angle").getValue(Float.class);
                        Float distanceMeters = dataSnapshot.child("distance").getValue(Float.class); // ESP32 gửi mét

                        Log.d(TAG, "Parsed angle: " + angle);
                        Log.d(TAG, "Parsed distanceMeters: " + distanceMeters);

                        if (angle != null && distanceMeters != null && radarView != null) {
                            final float finalAngle = angle;
                            final float finalDistanceMeters = distanceMeters;
                            Log.d(TAG, "Calling onSensorDataReceived with Angle: " + finalAngle + ", Distance (meters): " + finalDistanceMeters);
                            runOnUiThread(() -> onSensorDataReceived(finalAngle, finalDistanceMeters));
                        } else {
                            if (angle == null) Log.w(TAG, "Angle from Firebase is null.");
                            if (distanceMeters == null) Log.w(TAG, "DistanceMeters from Firebase is null.");
                            if (radarView == null) Log.w(TAG, "radarView is null.");
                        }
                    } else {
                        Log.w(TAG, "RadarData snapshot does NOT exist.");
                    }
                } catch (Exception e) {
                    Log.e(TAG, "Lỗi đọc dữ liệu radar từ Firebase", e);
                }
            }
            @Override
            public void onCancelled(@NonNull DatabaseError databaseError) {
                Log.w(TAG, "Lỗi lắng nghe Firebase cho radar: ", databaseError.toException());
            }
        });

        DatabaseReference obstacleStatusRef = RootRef.child("SensorStatus").child("ObstacleDetected");
        obstacleStatusRef.addValueEventListener(new ValueEventListener() {
            @Override
            public void onDataChange(@NonNull DataSnapshot dataSnapshot) {
                try {
                    if (dataSnapshot.exists()) {
                        Boolean isObstacle = dataSnapshot.getValue(Boolean.class);
                        if (isObstacle != null && radioButtonObstacleDetected != null) {
                            final boolean obstacleFinal = isObstacle;
                            runOnUiThread(() -> {
                                radioButtonObstacleDetected.setChecked(obstacleFinal);
                                if (textViewNotification != null) {
                                    textViewNotification.setText(obstacleFinal ? "Thông báo: CÓ VẬT CẢN!" : "Thông báo: Đường thoáng");
                                }
                            });
                        }
                    }
                } catch (Exception e) {
                    Log.e(TAG, "Lỗi đọc ObstacleDetected từ Firebase", e);
                }
            }
            @Override
            public void onCancelled(@NonNull DatabaseError databaseError) {
                Log.w(TAG, "Lỗi lắng nghe Firebase cho ObstacleDetected: ", databaseError.toException());
            }
        });

        DatabaseReference currentDistanceCmRef = RootRef.child("SensorStatus").child("CurrentDistanceCm");
        currentDistanceCmRef.addValueEventListener(new ValueEventListener() {
            @Override
            public void onDataChange(@NonNull DataSnapshot dataSnapshot) {
                try {
                    if (dataSnapshot.exists()) {
                        Object value = dataSnapshot.getValue();
                        Float distanceCm = null;

                        if (value instanceof Float) {
                            distanceCm = (Float) value;
                        } else if (value instanceof Double) {
                            distanceCm = ((Double) value).floatValue();
                        } else if (value instanceof Long) {
                            distanceCm = ((Long) value).floatValue();
                        } else if (value instanceof String) {
                            try {
                                distanceCm = Float.parseFloat((String) value);
                            } catch (NumberFormatException nfe) {
                                Log.e(TAG, "Không thể parse String khoảng cách thành Float: " + value, nfe);
                            }
                        }

                        if (distanceCm != null && textViewObstacleDistance != null) {
                            final float finalDistanceCm = distanceCm;
                            runOnUiThread(() -> {
                                if (finalDistanceCm >= 0 && finalDistanceCm < 999.0) { // Kiểm tra giá trị hợp lệ
                                    textViewObstacleDistance.setText(String.format(Locale.getDefault(), "%.1f cm", finalDistanceCm));
                                } else {
                                    textViewObstacleDistance.setText("-- cm (Lỗi/Ngoài tầm)");
                                }
                            });
                        } else if (distanceCm == null) {
                            Log.w(TAG, "Giá trị khoảng cách (cm) từ Firebase là null hoặc không thể chuyển đổi: " + value);
                        }
                    }
                } catch (Exception e) {
                    Log.e(TAG, "Lỗi đọc CurrentDistanceCm từ Firebase", e);
                }
            }
            @Override
            public void onCancelled(@NonNull DatabaseError databaseError) {
                Log.w(TAG, "Lỗi lắng nghe Firebase cho CurrentDistanceCm: ", databaseError.toException());
            }
        });
    }
}
