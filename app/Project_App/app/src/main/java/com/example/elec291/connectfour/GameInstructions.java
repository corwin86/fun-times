package com.example.elec291.connectfour;

import android.app.Activity;
import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.util.DisplayMetrics;
import android.view.View;
import android.widget.Button;

/**
 * Created by rohini on 24/03/16.
 */
public class GameInstructions extends AppCompatActivity implements View.OnClickListener{
    Button buttonDone;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.content_game_instructions);
        setTitle("How to Play");

        DisplayMetrics dm = new DisplayMetrics();
        getWindowManager().getDefaultDisplay().getMetrics(dm);

        int width = dm.widthPixels;
        int height = dm.heightPixels;

        getWindow().setLayout((int) (width * 0.8), (int) (height * 0.5));

        buttonDone = (Button) findViewById(R.id.buttonDone);
        buttonDone.setOnClickListener(this);
    }

    @Override
    public void onClick(View v){
        switch((v.getId())){
            case R.id.buttonDone:
                finish();
        }
    }
}


