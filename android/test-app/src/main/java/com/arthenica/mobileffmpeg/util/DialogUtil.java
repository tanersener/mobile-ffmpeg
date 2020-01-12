/*
 * Copyright (c) 2019 Alexander Berezhnoi
 *
 * This file is part of MobileFFmpeg.
 *
 * MobileFFmpeg is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * MobileFFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with MobileFFmpeg.  If not, see <http://www.gnu.org/licenses/>.
 */

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
