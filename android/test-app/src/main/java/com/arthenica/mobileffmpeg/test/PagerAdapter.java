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

import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentManager;
import androidx.fragment.app.FragmentPagerAdapter;

public class PagerAdapter extends FragmentPagerAdapter {
    private final MainActivity mainActivity;
    private int numberOfTabs;

    PagerAdapter(final FragmentManager fragmentManager, final MainActivity mainActivity, final int numberOfTabs) {
        super(fragmentManager);

        this.mainActivity = mainActivity;
        this.numberOfTabs = numberOfTabs;
    }

    @Override
    public Fragment getItem(final int position) {
        switch (position) {
            case 0: {
                return CommandTabFragment.newInstance(mainActivity);
            }
            case 1: {
                return VideoTabFragment.newInstance(mainActivity);
            }
            case 2: {
                return HttpsTabFragment.newInstance(mainActivity);
            }
            case 3: {
                return AudioTabFragment.newInstance(mainActivity);
            }
            case 4: {
                return SubtitleTabFragment.newInstance(mainActivity);
            }
            case 5: {
                return VidStabTabFragment.newInstance(mainActivity);
            }
            case 6: {
                return PipeTabFragment.newInstance(mainActivity);
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
    public CharSequence getPageTitle(final int position) {
        switch (position) {
            case 0: {
                return mainActivity.getString(R.string.command_tab);
            }
            case 1: {
                return mainActivity.getString(R.string.video_tab);
            }
            case 2: {
                return mainActivity.getString(R.string.https_tab);
            }
            case 3: {
                return mainActivity.getString(R.string.audio_tab);
            }
            case 4: {
                return mainActivity.getString(R.string.subtitle_tab);
            }
            case 5: {
                return mainActivity.getString(R.string.vidstab_tab);
            }
            case 6: {
                return mainActivity.getString(R.string.pipe_tab);
            }
            default: {
                return null;
            }
        }
    }

}
