/*
 * Copyright (c) 2018 Taner Sener
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
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with MobileFFmpeg.  If not, see <http://www.gnu.org/licenses/>.
 */

package com.arthenica.mobileffmpeg.test;

import android.content.Context;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentManager;
import android.support.v4.app.FragmentPagerAdapter;

public class PagerAdapter extends FragmentPagerAdapter {
    private Context context;
    private int numberOfTabs;

    PagerAdapter(FragmentManager fragmentManager, Context context, int numberOfTabs) {
        super(fragmentManager);

        this.context = context;
        this.numberOfTabs = numberOfTabs;
    }

    @Override
    public Fragment getItem(int position) {
        switch (position) {
            case 0: {
                return new CommandTabFragment();
            }
            case 1: {
                return new SlideshowTabFragment();
            }
            default: {
                return null;
            }
        }
    }

    @Override
    public int getCount() {
        return numberOfTabs;
    }

    @Override
    public CharSequence getPageTitle(int position) {
        switch (position) {
            case 0: {
                return context.getString(R.string.command_tab);
            }
            case 1: {
                return context.getString(R.string.slideshow_tab);
            }
            default: {
                return null;
            }
        }
    }

}
