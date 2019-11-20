package com.arthenica.mobileffmpeg.util;

import android.app.AlertDialog;
import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

import com.arthenica.mobileffmpeg.test.R;

public class DialogUtil {

    public static AlertDialog createProgressDialog(Context context, final String text) {
        AlertDialog.Builder builder = new AlertDialog.Builder(context);
        LayoutInflater inflater = (LayoutInflater) context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        if (inflater != null) {
            View dialogView = inflater.inflate(R.layout.progress_dialog_layout, null);
            builder.setView(dialogView);

            TextView textView = dialogView.findViewById(R.id.progressDialogText);
            if (textView != null) {
                textView.setText(text);
            }
        }
        builder.setCancelable(false);
        return builder.create();
    }

    public static AlertDialog createCancellableProgressDialog(Context context, final String text, final View.OnClickListener onClickListener) {
        AlertDialog.Builder builder = new AlertDialog.Builder(context);
        LayoutInflater inflater = (LayoutInflater) context.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        if (inflater != null) {
            View dialogView = inflater.inflate(R.layout.cancellable_progress_dialog_layout, null);
            builder.setView(dialogView);

            TextView textView = dialogView.findViewById(R.id.progressDialogText);
            if (textView != null) {
                textView.setText(text);
            }
            Button cancelButton = dialogView.findViewById(R.id.cancelButton);
            if (cancelButton != null) {
                cancelButton.setOnClickListener(onClickListener);
            }
        }
        builder.setCancelable(false);
        return builder.create();
    }

}
