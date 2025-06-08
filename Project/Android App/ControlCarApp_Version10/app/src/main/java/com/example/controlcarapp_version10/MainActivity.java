package com.example.controlcarapp_version10;

import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.Toast;

import androidx.activity.EdgeToEdge;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.graphics.Insets;
import androidx.core.view.ViewCompat;
import androidx.core.view.WindowInsetsCompat;

public class MainActivity extends AppCompatActivity {

    private static final String TAG = "MainActivity";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        EdgeToEdge.enable(this);
        setContentView(R.layout.activity_main);

        // Đoạn code xử lý WindowInsets của bạn (giữ nguyên)
        ViewCompat.setOnApplyWindowInsetsListener(findViewById(R.id.main), (v, insets) -> {
            Insets systemBars = insets.getInsets(WindowInsetsCompat.Type.systemBars());
            v.setPadding(systemBars.left, systemBars.top, systemBars.right, systemBars.bottom);
            return insets;
        });

        // --- BẮT ĐẦU PHẦN CODE THÊM ĐỂ XỬ LÝ SỰ KIỆN NHẤN NÚT ---

        // 1. Tìm Button "Remote Control" bằng ID của nó
        Button buttonRemoteControl = findViewById(R.id.buttonRemoteControl);

        // 2. Kiểm tra xem Button có được tìm thấy không (thực hành tốt)
        if (buttonRemoteControl == null) {
            Log.e(TAG, "Không tìm thấy Button với ID 'buttonRemoteControl' trong activity_main.xml!");
            Toast.makeText(this, "Lỗi: Không tìm thấy nút Remote Control!", Toast.LENGTH_LONG).show();
        } else {
            // 3. Thiết lập OnClickListener cho Button
            buttonRemoteControl.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    // Ghi log và hiển thị Toast để xác nhận sự kiện được kích hoạt
                    Log.d(TAG, "Nút Remote Control đã được nhấn!");
                    Toast.makeText(MainActivity.this, "Đang mở Remote Control...", Toast.LENGTH_SHORT).show();

                    // 4. Tạo một Intent để khởi động RemoteControlActivity
                    Intent intent = new Intent(MainActivity.this, RemoteControlActivity.class);

                    // 5. Bắt đầu Activity mới
                    startActivity(intent);
                }
            });
        }
        // --- KẾT THÚC PHẦN CODE THÊM ---
    }
}