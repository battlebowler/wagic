package org.libsdl.app;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.ProgressDialog;

import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;

import android.content.pm.PackageManager.NameNotFoundException;

import android.content.res.Configuration;

import android.graphics.Canvas;
import android.graphics.PixelFormat;

import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;

import android.net.Uri;

import android.os.AsyncTask;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Message;
import android.os.StrictMode;

import android.util.Log;

import android.view.KeyEvent;
import android.view.Menu;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.SubMenu;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.VelocityTracker;
import android.view.View;

import android.view.View.OnKeyListener;

import android.view.WindowManager;

import android.widget.FrameLayout;
import android.widget.FrameLayout.LayoutParams;
import android.widget.PopupMenu;

import net.wagic.app.R;

import net.wagic.utils.DeckImporter;
import net.wagic.utils.ImgDownloader;
import net.wagic.utils.StorageOptions;

import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

import java.net.URL;
import java.net.URLConnection;

import java.util.ArrayList;
import java.util.Enumeration;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;
import java.util.zip.ZipOutputStream;

import javax.microedition.khronos.egl.EGL10;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.egl.EGLContext;
import javax.microedition.khronos.egl.EGLDisplay;
import javax.microedition.khronos.egl.EGLSurface;


public class SDLActivity extends Activity implements OnKeyListener {
    private static final String TAG = SDLActivity.class.getCanonicalName();

    // Main components
    private static SDLActivity mSingleton;
    private static SDLSurface mSurface;

    // Audio
    private static Thread mAudioThread;
    private static AudioTrack mAudioTrack;

    // Resource download
    public static final int DIALOG_DOWNLOAD_PROGRESS = 0;
    public static final int DIALOG_DOWNLOAD_ERROR = 1;

    public static String RES_FILENAME = "";
    public static String databaseurl = "https://github.com/WagicProject/wagic/releases/latest/download/CardImageLinks.csv";

    // Preferences
    public static final String kWagicSharedPreferencesKey = "net.wagic.app.preferences.wagic";
    public static final String kStoreDataOnRemovableSdCardPreference = "StoreDataOnRemovableStorage";
    public static final String kSaveDataPathPreference = "StorageDataLocation";
    public static final String kWagicDataStorageOptionsKey = "dataStorageOptions";
    public static final int kStorageDataOptionsMenuId = 2000;
    public static final int kdownloadResOptionsMenuId = 4000;
    public static final int kOtherOptionsMenuId = 3000;

    // Admin request codes
    private static final int REQUEST_PICK_ZIP  = 9001;
    private static final int REQUEST_EXPORT_ZIP = 9002;

    static {
        System.loadLibrary("SDL");
        System.loadLibrary("main");
    }

    static int COMMAND_CHANGE_TITLE = 1;
    static int COMMAND_JGE_MSG = 2;

    private static Object buf;

    // Import deck globals
    public ArrayList<String> myresult = new ArrayList<String>();
    public String myclickedItem = "";
    private ProgressDialog mProgressDialog;
    private AlertDialog mErrorDialog;
    public String mErrorMessage = "";
    public Boolean mErrorHappened = false;
    public String systemFolder = Environment.getExternalStorageDirectory().getPath() + "/Wagic/Res/";
    private String userFolder = Environment.getExternalStorageDirectory().getPath() + "/Wagic/User/";
    private String internalPath = "";
    private String sdcardPath = "";

    private Context mContext;
    String set = "";
    String[] availableSets;
    ArrayList<String> selectedSets;
    boolean[] checkedSet;
    Integer totalset = 0;
    boolean finished = false;
    boolean loadResInProgress = false;
    ProgressDialog progressBarDialogRes;
    boolean fast = false;
    String targetRes = "High";
    boolean error = false;
    boolean skipDownloaded = false;
    boolean borderless = false;
    String res = "";
    public volatile boolean downloadInProgress = false;
    public volatile boolean paused = false;
    ProgressDialog cardDownloader;
    volatile int currentIndex = 0;
    MenuItem importDecks;
    MenuItem downloader;
    MenuItem about;
    MenuItem storage;
    MenuItem resource;

    Handler commandHandler = new Handler() {
        public void handleMessage(Message msg) {
            if (msg.arg1 == COMMAND_CHANGE_TITLE) {
                setTitle((String) msg.obj);
            } else if (msg.arg1 == COMMAND_JGE_MSG) {
                processJGEMsg((String) msg.obj);
            }
        }
    };

    // =========================================================================
    // Admin menu
    // =========================================================================

    public void showAdminMenu() {
        final String[] options = {
                "Import Data",
                "Export Data",
                "Download Cards",
                "About"
        };

        new AlertDialog.Builder(this)
                .setTitle("Admin Panel")
                .setItems(options, new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        switch (which) {
                            case 0: adminPickZip();    break;
                            case 1: adminExportData(); break;
                            case 2:
                                if (availableSets == null) loadAvailableSets();
                                else if (loadResInProgress) progressBarDialogRes.show();
                                else if (downloadInProgress) cardDownloader.show();
                                else downloadCardImages();
                                break;
                            case 3: showAbout(); break;
                        }
                    }
                })
                .setNegativeButton("Cancel", null)
                .show();
    }

    // --- Import ---

    private void adminPickZip() {
        Intent intent = new Intent(Intent.ACTION_GET_CONTENT);
        intent.setType("application/zip");
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        try {
            startActivityForResult(Intent.createChooser(intent, "Select a zip file"), REQUEST_PICK_ZIP);
        } catch (android.content.ActivityNotFoundException e) {
            new AlertDialog.Builder(this)
                    .setTitle("No file manager found")
                    .setMessage("Please install a file manager app and try again.")
                    .setPositiveButton("OK", null)
                    .show();
        }
    }

    private void adminImportZip(final Uri uri) {
        final ProgressDialog progress = new ProgressDialog(this);
        progress.setTitle("Importing data...");
        progress.setMessage("Please wait...");
        progress.setIndeterminate(true);
        progress.setCancelable(false);
        progress.show();

        final Handler handler = new Handler();

        new Thread(new Runnable() {
            @Override
            public void run() {
                String resultMessage;
                try {
                    File dest = new File(getUserStorageLocation());
                    dest.mkdirs();

                    InputStream is = getContentResolver().openInputStream(uri);
                    java.util.zip.ZipInputStream zis =
                            new java.util.zip.ZipInputStream(new BufferedInputStream(is));

                    ZipEntry entry;
                    byte[] buffer = new byte[8192];

                    while ((entry = zis.getNextEntry()) != null) {
                        File outFile = new File(dest, entry.getName());
                        if (entry.isDirectory()) {
                            outFile.mkdirs();
                        } else {
                            outFile.getParentFile().mkdirs();
                            FileOutputStream fos = new FileOutputStream(outFile);
                            int count;
                            while ((count = zis.read(buffer)) != -1) {
                                fos.write(buffer, 0, count);
                            }
                            fos.close();
                        }
                        zis.closeEntry();
                    }
                    zis.close();
                    resultMessage = "Import completed!\n\nDestination: " + getUserStorageLocation();
                } catch (Exception e) {
                    Log.e(TAG, "Admin import failed: " + e.getMessage());
                    resultMessage = "Import failed:\n" + e.getMessage();
                }

                final String msg = resultMessage;
                handler.post(new Runnable() {
                    @Override
                    public void run() {
                        progress.dismiss();
                        new AlertDialog.Builder(SDLActivity.this)
                                .setTitle("Import Result")
                                .setMessage(msg)
                                .setPositiveButton("OK", null)
                                .show();
                    }
                });
            }
        }).start();
    }

    // --- Export ---

    private void adminExportData() {
        Intent intent = new Intent(Intent.ACTION_CREATE_DOCUMENT);
        intent.setType("application/zip");
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.putExtra(Intent.EXTRA_TITLE, "wagic_user_data.zip");
        try {
            startActivityForResult(Intent.createChooser(intent, "Save user data as..."), REQUEST_EXPORT_ZIP);
        } catch (android.content.ActivityNotFoundException e) {
            new AlertDialog.Builder(this)
                    .setTitle("No file manager found")
                    .setMessage("Please install a file manager app and try again.")
                    .setPositiveButton("OK", null)
                    .show();
        }
    }

    private void adminExportZip(final Uri uri) {
        final ProgressDialog progress = new ProgressDialog(this);
        progress.setTitle("Exporting user data...");
        progress.setMessage("Please wait...");
        progress.setIndeterminate(true);
        progress.setCancelable(false);
        progress.show();

        final Handler handler = new Handler();

        new Thread(new Runnable() {
            @Override
            public void run() {
                String resultMessage;
                try {
                    File sourceDir = new File(getUserStorageLocation());
                    OutputStream os = getContentResolver().openOutputStream(uri);
                    ZipOutputStream zos = new ZipOutputStream(os);
                    zipDirectory(sourceDir, sourceDir, zos);
                    zos.close();
                    os.close();
                    resultMessage = "Export completed!";
                } catch (Exception e) {
                    Log.e(TAG, "Admin export failed: " + e.getMessage());
                    resultMessage = "Export failed:\n" + e.getMessage();
                }

                final String msg = resultMessage;
                handler.post(new Runnable() {
                    @Override
                    public void run() {
                        progress.dismiss();
                        new AlertDialog.Builder(SDLActivity.this)
                                .setTitle("Export Result")
                                .setMessage(msg)
                                .setPositiveButton("OK", null)
                                .show();
                    }
                });
            }
        }).start();
    }

    private void zipDirectory(File baseDir, File currentDir, ZipOutputStream zos) throws IOException {
        File[] files = currentDir.listFiles();
        if (files == null) return;
        byte[] buffer = new byte[8192];
        for (File file : files) {
            if (file.isDirectory()) {
                zipDirectory(baseDir, file, zos);
            } else {
                String entryName = baseDir.toURI().relativize(file.toURI()).getPath();
                zos.putNextEntry(new ZipEntry(entryName));
                FileInputStream fis = new FileInputStream(file);
                int count;
                while ((count = fis.read(buffer)) != -1) {
                    zos.write(buffer, 0, count);
                }
                fis.close();
                zos.closeEntry();
            }
        }
    }

    // --- Activity result ---

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (resultCode != RESULT_OK || data == null) return;
        Uri uri = data.getData();
        if (uri == null) return;
        if (requestCode == REQUEST_PICK_ZIP) {
            adminImportZip(uri);
        } else if (requestCode == REQUEST_EXPORT_ZIP) {
            adminExportZip(uri);
        }
    }

    private void showAbout() {
        new AlertDialog.Builder(this)
                .setTitle("Wagic Info")
                .setMessage("Version: " +
                        getResources().getString(R.string.app_version) + "\r\n" +
                        getResources().getString(R.string.info_text))
                .setPositiveButton("OK", null)
                .show();
    }

    // =========================================================================
    // Storage
    // =========================================================================

    public String getSystemStorageLocation() { return systemFolder; }
    public String getUserStorageLocation()   { return userFolder; }

    public void updateStorageLocations() {
        boolean usesInternalSdCard = (!getSharedPreferences(kWagicSharedPreferencesKey, MODE_PRIVATE)
                .getBoolean(kStoreDataOnRemovableSdCardPreference, false)) &&
                Environment.MEDIA_MOUNTED.equals(Environment.getExternalStorageState());
        systemFolder = (usesInternalSdCard ? internalPath : sdcardPath) + "/Res/";
        userFolder   = (usesInternalSdCard ? internalPath : sdcardPath) + "/User/";
    }

    public boolean checkStorageState() {
        SharedPreferences settings = getSharedPreferences(kWagicSharedPreferencesKey, MODE_PRIVATE);
        boolean mExternalStorageAvailable = false;
        boolean mExternalStorageWriteable = false;
        String state = Environment.getExternalStorageState();
        boolean useSdCard = (!settings.getBoolean(kStoreDataOnRemovableSdCardPreference, false)) && mExternalStorageWriteable;
        String systemStoragePath = getSystemStorageLocation();
        if (useSdCard  && systemStoragePath.indexOf(sdcardPath)  != -1) return true;
        if (!useSdCard && systemStoragePath.indexOf(internalPath) != -1) return true;
        if (Environment.MEDIA_MOUNTED.equals(state)) {
            mExternalStorageAvailable = mExternalStorageWriteable = true;
        } else if (Environment.MEDIA_MOUNTED_READ_ONLY.equals(state)) {
            mExternalStorageAvailable = true;
        } else {
            mExternalStorageAvailable = mExternalStorageWriteable = false;
        }
        return mExternalStorageAvailable && mExternalStorageWriteable;
    }

    private boolean getRemovableMediaStorageState() {
        for (String extMediaPath : StorageOptions.paths) {
            if (new File(extMediaPath).canWrite()) return true;
        }
        return false;
    }

    private void displayStorageOptions() {
        AlertDialog.Builder setStorage = new AlertDialog.Builder(this);
        setStorage.setTitle("Where would you like to store your data? On your removable SD Card or the built-in memory?");
        StorageOptions.determineStorageOptions(mContext);

        SharedPreferences settings = getSharedPreferences(kWagicSharedPreferencesKey, MODE_PRIVATE);
        String selectedPath = settings.getString(kSaveDataPathPreference, "");
        int selectedIndex = -1;
        for (int i = 0; i < StorageOptions.labels.length; i++) {
            if (StorageOptions.labels[i].contains(selectedPath)) { selectedIndex = i; break; }
        }

        setStorage.setSingleChoiceItems(StorageOptions.labels, selectedIndex,
                new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int item) { savePathPreference(item); }
                });
        setStorage.setPositiveButton("OK", new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int which) {
                initStorage();
                if (mSurface == null) mSingleton.initializeGame();
            }
        });
        setStorage.create().show();
    }

    private void importDeckOptions() {
        AlertDialog.Builder importDeck = new AlertDialog.Builder(this);
        importDeck.setTitle("Choose Deck to Import:");

        File root = new File(System.getenv("EXTERNAL_STORAGE") + "/Download");
        File[] files = root.listFiles();
        if (files != null) {
            for (File f : files) {
                if (!myresult.contains(f.toString()) &&
                        (f.toString().contains(".txt") || f.toString().contains(".dck") || f.toString().contains(".dec"))) {
                    myresult.add(f.toString());
                }
            }
        }
        if (!myresult.isEmpty()) myclickedItem = myresult.get(0).toString();

        importDeck.setSingleChoiceItems(myresult.toArray(new String[myresult.size()]), 0,
                new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int item) {
                        myclickedItem = myresult.get(item).toString();
                    }
                });
        importDeck.setPositiveButton("Import Deck", new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int which) {
                processSelectedDeck(myclickedItem);
                if (mSurface == null) mSingleton.initializeGame();
            }
        });
        importDeck.create().show();
    }

    private void processSelectedDeck(String mypath) {
        AlertDialog.Builder infoDialog = new AlertDialog.Builder(this);
        infoDialog.setTitle("Imported Deck:");
        String activePath = sdcardPath.isEmpty() ? internalPath : sdcardPath;
        String state = DeckImporter.importDeck(new File(mypath), mypath, activePath);
        infoDialog.setMessage(state);
        infoDialog.show();
    }

    private void checkStorageLocationPreference() {
        SharedPreferences settings = getSharedPreferences(kWagicSharedPreferencesKey, MODE_PRIVATE);
        final SharedPreferences.Editor prefsEditor = settings.edit();
        boolean hasRemovableMediaMounted = getRemovableMediaStorageState();

        if (!settings.contains(kStoreDataOnRemovableSdCardPreference)) {
            if (hasRemovableMediaMounted) {
                displayStorageOptions();
            } else {
                prefsEditor.putBoolean(kStoreDataOnRemovableSdCardPreference, false);
                prefsEditor.commit();
                initStorage();
                mSingleton.initializeGame();
            }
        } else {
            boolean storeOnRemovableMedia = settings.getBoolean(kStoreDataOnRemovableSdCardPreference, false);
            if (storeOnRemovableMedia && !hasRemovableMediaMounted) {
                AlertDialog setStorage = new AlertDialog.Builder(this).create();
                setStorage.setTitle("Storage Preference");
                setStorage.setMessage("Removable Sd Card not detected. Saving data to internal memory.");
                prefsEditor.putBoolean(kStoreDataOnRemovableSdCardPreference, false);
                prefsEditor.commit();
                initStorage();
                mSingleton.initializeGame();
                setStorage.show();
            } else {
                initStorage();
                mSingleton.initializeGame();
            }
        }
    }

    private void initStorage() {
        try {
            File externalFilesDir = Environment.getExternalStorageDirectory();
            if (externalFilesDir != null) internalPath = externalFilesDir.getAbsolutePath() + "/Wagic";

            if (Environment.MEDIA_MOUNTED.equals(Environment.getExternalStorageState())) {
                File wagicMediaPath = new File(internalPath);
                if (wagicMediaPath.canWrite()) wagicMediaPath.mkdirs();
            }

            SharedPreferences settings = getSharedPreferences(kWagicSharedPreferencesKey, MODE_PRIVATE);
            String selectedRemovableCardPath = settings.getString(kSaveDataPathPreference, internalPath);
            if (selectedRemovableCardPath != null && !internalPath.equalsIgnoreCase(selectedRemovableCardPath)) {
                File wagicMediaPath = new File(selectedRemovableCardPath);
                if (!wagicMediaPath.exists() || !wagicMediaPath.canWrite()) {
                    Log.e(TAG, "Error in initializing system folder: " + selectedRemovableCardPath);
                } else {
                    sdcardPath = selectedRemovableCardPath + "/Wagic";
                }
            }
            updateStorageLocations();
        } catch (Exception ioex) {
            Log.e(TAG, "An error occurred in setting up the storage locations.");
        }
    }

    private void savePathPreference(int selectedOption) {
        SharedPreferences settings = getSharedPreferences(kWagicSharedPreferencesKey, MODE_PRIVATE);
        String selectedMediaPath = StorageOptions.paths[selectedOption];
        final SharedPreferences.Editor prefsEditor = settings.edit();
        prefsEditor.putBoolean(kStoreDataOnRemovableSdCardPreference, !"/mnt/sdcard".equalsIgnoreCase(selectedMediaPath));
        prefsEditor.putString(kSaveDataPathPreference, selectedMediaPath);
        prefsEditor.commit();
    }

    private void startDownload() {
        if (!checkStorageState()) {
            Log.e(TAG, "Error in initializing storage space.");
            mSingleton.downloadError("Failed to initialize storage space for game. Please verify that your sdcard or internal memory is mounted properly.");
        }
        new DownloadFileAsync().execute(getResourceUrl());
    }

    public void downloadError(String errorMessage) {
        mErrorHappened = true;
        mErrorMessage = errorMessage;
    }

    // =========================================================================
    // Card image downloader
    // =========================================================================

    private void loadAvailableSets() {
        final Handler mHandler = new Handler();
        progressBarDialogRes = new ProgressDialog(this);
        progressBarDialogRes.setTitle("Loading all available sets...");
        progressBarDialogRes.setProgressStyle(ProgressDialog.STYLE_HORIZONTAL);
        progressBarDialogRes.setProgress(0);

        new Thread(new Runnable() {
            public void run() {
                ArrayList<String> sets = new ArrayList<String>();
                if (availableSets == null) {
                    loadResInProgress = true;
                    ZipFile zipFile = null;
                    try {
                        zipFile = new ZipFile(new File(getSystemStorageLocation()) + "/" + RES_FILENAME);
                        Enumeration<? extends ZipEntry> e = zipFile.entries();
                        while (e.hasMoreElements()) {
                            ZipEntry entry = e.nextElement();
                            String entryName = entry.getName();
                            if (entryName != null && entryName.contains("sets/") &&
                                    !entryName.equalsIgnoreCase("sets/") &&
                                    !entryName.contains("primitives") &&
                                    !entryName.contains(".")) {
                                sets.add(entryName.split("/")[1]);
                            }
                        }
                    } catch (IOException ioe) {
                        System.out.println("Error opening zip file" + ioe);
                    } finally {
                        try { if (zipFile != null) zipFile.close(); } catch (IOException ioe) {}
                    }
                    availableSets = new String[sets.size()];
                    checkedSet = new boolean[sets.size()];
                    progressBarDialogRes.setMax(sets.size());
                    for (int i = 0; i < availableSets.length; i++) {
                        availableSets[i] = sets.get(i) + " - " +
                                ImgDownloader.getSetInfo(sets.get(i), true, getSystemStorageLocation());
                        checkedSet[i] = false;
                        progressBarDialogRes.incrementProgressBy(1);
                    }
                }
                finished = true;
                loadResInProgress = false;
                progressBarDialogRes.dismiss();
                mHandler.post(new Runnable() {
                    public void run() {
                        selectedSets = new ArrayList<String>();
                        showWarningFast();
                    }
                });
            }
        }).start();

        new Thread(new Runnable() {
            public void run() {
                fast = ImgDownloader.loadDatabase(getSystemStorageLocation(), databaseurl);
            }
        }).start();

        progressBarDialogRes.show();
    }

    private void showWarningFast() {
        AlertDialog.Builder infoDialog = new AlertDialog.Builder(this);
        if (!fast) {
            infoDialog.setTitle("Problem downloading the images database file");
            infoDialog.setMessage("The program will use the slow (not indexed) method, so the images download may take really long time...");
            infoDialog.setNegativeButton("Retry", new DialogInterface.OnClickListener() {
                public void onClick(DialogInterface dialog, int id) {
                    fast = ImgDownloader.loadDatabase(getSystemStorageLocation(), databaseurl);
                    showWarningFast();
                }
            });
        } else {
            infoDialog.setTitle("Images Database correctly downloaded");
            infoDialog.setMessage("The program will use the fast (indexed) method, so the images download will not take long time!");
        }
        infoDialog.setPositiveButton("Continue", new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int id) { downloadCardImages(); }
        });
        infoDialog.create().show();
    }

    private void downloadCardImages() {
        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        builder.setTitle("Which Sets would you like to download?");
        builder.setMultiChoiceItems(availableSets, checkedSet,
                new DialogInterface.OnMultiChoiceClickListener() {
                    public void onClick(DialogInterface dialog, int which, boolean isChecked) {
                        checkedSet[which] = isChecked;
                        if (isChecked) selectedSets.add(availableSets[which].split(" - ")[0]);
                        else selectedSets.remove(availableSets[which].split(" - ")[0]);
                    }
                });
        builder.setNeutralButton("Download All", new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int id) {
                selectedSets.clear();
                for (String s : availableSets) selectedSets.add(s.split(" - ")[0]);
                chooseResolution();
            }
        });
        builder.setPositiveButton("Download Selected", new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {}
        });
        final AlertDialog dialog = builder.create();
        dialog.show();
        dialog.getButton(AlertDialog.BUTTON_POSITIVE).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (selectedSets.size() > 0) { chooseResolution(); dialog.dismiss(); }
            }
        });
    }

    private void chooseResolution() {
        final String[] availableRes = {
                "High - (672x936)", "High - (672x936) - Borderless",
                "Medium - (488x680)", "Medium - (488x680) - Borderless",
                "Low - (244x340)", "Low - (244x340) - Borderless",
                "Tiny - (180x255)", "Tiny - (180x255) - Borderless"
        };
        new AlertDialog.Builder(this)
                .setTitle("Which resolution would you like to use?")
                .setSingleChoiceItems(availableRes, 0, new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int item) {
                        targetRes = availableRes[item].split(" - ")[0];
                        borderless = availableRes[item].split(" - ").length > 2;
                    }
                })
                .setPositiveButton("Start Download", new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int which) { skipDownloadedSets(); }
                })
                .setNegativeButton("Change Selection", new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int which) { downloadCardImages(); }
                })
                .create().show();
    }

    private void skipDownloadedSets() {
        new AlertDialog.Builder(this)
                .setTitle("Do you want to overwrite existing sets?")
                .setPositiveButton("Yes", new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int which) { skipDownloaded = false; downloadCardImagesStart(); }
                })
                .setNegativeButton("No", new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int which) { skipDownloaded = true; downloadCardImagesStart(); }
                })
                .create().show();
    }

    private void downloadCardImagesStart() {
        final SDLActivity parent = this;
        final Handler mHandler = new Handler();
        cardDownloader = new ProgressDialog(this);
        cardDownloader.setTitle("Downloading now set: " + set);
        cardDownloader.setProgressStyle(ProgressDialog.STYLE_HORIZONTAL);
        cardDownloader.setProgress(0);
        cardDownloader.setMessage(selectedSets.size() == 1
                ? "You choose to download just 1 set: Please don't quit Wagic or turn off Internet connection."
                : "You choose to download " + selectedSets.size() + " sets: Please don't quit Wagic or turn off Internet connection.");

        new Thread(new Runnable() {
            public void run() {
                downloadInProgress = true;
                paused = false;
                if (selectedSets != null) {
                    for (currentIndex = 0; currentIndex < selectedSets.size() && downloadInProgress; currentIndex++) {
                        while (paused) {
                            try { Thread.sleep(1000); } catch (InterruptedException e) {}
                            if (!downloadInProgress) break;
                        }
                        try {
                            set = selectedSets.get(currentIndex);
                            mHandler.post(new Runnable() {
                                public void run() {
                                    cardDownloader.setTitle("Downloading set: " + set +
                                            " (" + (currentIndex + 1) + " of " + selectedSets.size() + ")");
                                }
                            });
                            String details = ImgDownloader.DownloadCardImages(set, availableSets, targetRes,
                                    getSystemStorageLocation(), getUserStorageLocation() + "sets/",
                                    cardDownloader, parent, skipDownloaded, borderless);
                            if (!details.isEmpty()) {
                                res = res.isEmpty() ? "SET " + set + ":\n" + details : res + "\nSET " + set + ":\n" + details;
                            }
                        } catch (Exception e) {
                            res += "\n" + e.getMessage();
                            error = true;
                        }
                    }
                    mHandler.post(new Runnable() {
                        public void run() {
                            if (downloadInProgress) {
                                downloadSelectedSetsCompleted(error, res);
                                downloadInProgress = false;
                                paused = false;
                            }
                            cardDownloader.dismiss();
                        }
                    });
                }
            }
        }).start();

        cardDownloader.setButton(DialogInterface.BUTTON_POSITIVE, "Hide",
                new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int which) { cardDownloader.hide(); }
                });
        cardDownloader.setButton(DialogInterface.BUTTON_NEGATIVE, "Stop",
                new DialogInterface.OnClickListener() {
                    public void onClick(final DialogInterface dialog, int which) {
                        mHandler.post(new Runnable() {
                            public void run() {
                                downloadCardInterruped(set, cardDownloader.getProgress(), cardDownloader.getMax());
                                downloadInProgress = false;
                                paused = false;
                                ((AlertDialog) dialog).getButton(AlertDialog.BUTTON_NEUTRAL).setText("Pause");
                                cardDownloader.setTitle("Downloading now set: " + set + " - Interrupted");
                                cardDownloader.dismiss();
                            }
                        });
                    }
                });
        cardDownloader.setButton(DialogInterface.BUTTON_NEUTRAL, "Pause",
                new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {}
                });

        final AlertDialog dialog = (AlertDialog) cardDownloader;
        cardDownloader.show();
        dialog.getButton(AlertDialog.BUTTON_NEUTRAL).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                paused = !paused;
                dialog.getButton(AlertDialog.BUTTON_NEUTRAL).setText(paused ? "Resume" : "Pause");
                cardDownloader.setTitle("Downloading now set: " + set + (paused ? " - Paused" : ""));
            }
        });
    }

    private void downloadCardInterruped(String set, int cardsDownloaded, int total) {
        new AlertDialog.Builder(this)
                .setTitle("Download of " + set + " has been interrupted!")
                .setMessage("WARNING: Only " + cardsDownloaded + " of " + total +
                        " total cards have been downloaded. You have to start the download again to complete the set.")
                .setPositiveButton("OK", new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int which) { downloadCardImages(); }
                })
                .create().show();
        resetDownloadState();
    }

    private void downloadSelectedSetsCompleted(boolean error, String res) {
        AlertDialog.Builder infoDialog = new AlertDialog.Builder(this);
        if (!error) {
            infoDialog.setTitle("The download process has completed without any error");
            if (!res.isEmpty()) infoDialog.setMessage("Following cards could not be downloaded:\n" + res);
        } else {
            infoDialog.setTitle("Some errors occurred during the process!");
            infoDialog.setMessage(res);
        }
        resetDownloadState();
        infoDialog.create().show();
    }

    private void resetDownloadState() {
        res = "";
        set = "";
        targetRes = "High";
        skipDownloaded = false;
        borderless = false;
        currentIndex = 0;
        selectedSets = new ArrayList<String>();
        if (checkedSet != null) for (int i = 0; i < checkedSet.length; i++) checkedSet[i] = false;
        error = false;
    }

    // =========================================================================
    // Options menu (down swipe)
    // =========================================================================

    public void prepareOptionMenu(Menu menu) {
        if (menu == null) {
            PopupMenu p = new PopupMenu(mContext, null);
            menu = p.getMenu();
        }
        SubMenu settingsMenu = menu.addSubMenu(Menu.NONE, 1, 1, "Settings");
        importDecks = menu.add(Menu.NONE, 2, 2, "Import Decks");
        downloader  = menu.add(Menu.NONE, 3, 3, "Download Cards");
        about       = menu.add(Menu.NONE, 4, 4, "About");
        storage  = settingsMenu.add(kStorageDataOptionsMenuId, kStorageDataOptionsMenuId, Menu.NONE, "Storage Data Options");
        resource = settingsMenu.add(kdownloadResOptionsMenuId, kdownloadResOptionsMenuId, Menu.NONE, "Download Core & Quit");
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        prepareOptionMenu(menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        int itemId = item.getItemId();
        if (itemId == kStorageDataOptionsMenuId) {
            displayStorageOptions();
        } else if (itemId == kdownloadResOptionsMenuId) {
            new File(getSystemStorageLocation() + RES_FILENAME).delete();
            startDownload();
        } else if (itemId == 2) {
            importDeckOptions();
        } else if (itemId == 3) {
            if (availableSets == null) loadAvailableSets();
            else if (loadResInProgress) progressBarDialogRes.show();
            else if (downloadInProgress) cardDownloader.show();
            else downloadCardImages();
        } else if (itemId == 4) {
            showAbout();
        } else {
            return super.onOptionsItemSelected(item);
        }
        return true;
    }

    public void showSettingsSubMenu() {
        new AlertDialog.Builder(this)
                .setTitle("Settings Menu")
                .setItems(new String[]{"Storage Data Options", "Download Core & Quit"},
                        new DialogInterface.OnClickListener() {
                            @Override
                            public void onClick(DialogInterface dialog, int which) {
                                if (which == 0) onOptionsItemSelected(storage);
                                else onOptionsItemSelected(resource);
                            }
                        })
                .create().show();
    }

    public void showOptionMenu() {
        new AlertDialog.Builder(this)
                .setTitle("Options Menu")
                .setItems(new String[]{"Settings", "Import Decks", "Download Cards", "About"},
                        new DialogInterface.OnClickListener() {
                            @Override
                            public void onClick(DialogInterface dialog, int which) {
                                switch (which) {
                                    case 0: showSettingsSubMenu();              break;
                                    case 1: onOptionsItemSelected(importDecks); break;
                                    case 2: onOptionsItemSelected(downloader);  break;
                                    case 3: onOptionsItemSelected(about);       break;
                                }
                            }
                        })
                .create().show();
    }

    // =========================================================================
    // Dialogs
    // =========================================================================

    @Override
    protected Dialog onCreateDialog(int id) {
        switch (id) {
            case DIALOG_DOWNLOAD_PROGRESS:
                mProgressDialog = new ProgressDialog(this);
                mProgressDialog.setMessage("Downloading resource files (" + RES_FILENAME + ")");
                mProgressDialog.setProgressStyle(ProgressDialog.STYLE_HORIZONTAL);
                mProgressDialog.setCancelable(false);
                mProgressDialog.show();
                return mProgressDialog;

            case DIALOG_DOWNLOAD_ERROR:
                AlertDialog.Builder builder = new AlertDialog.Builder(this);
                builder.setMessage(mErrorMessage).setCancelable(false)
                        .setPositiveButton("Exit", new DialogInterface.OnClickListener() {
                            public void onClick(DialogInterface dialog, int id) { System.exit(0); }
                        });
                mErrorDialog = builder.create();
                mErrorDialog.show();
                return mErrorDialog;

            default:
                return null;
        }
    }

    // =========================================================================
    // Display / lifecycle
    // =========================================================================

    public void mainDisplay() {
        FrameLayout layout = new FrameLayout(this);
        mSurface = new SDLSurface(getApplication(), this);
        mSurface.getHolder().setType(SurfaceHolder.SURFACE_TYPE_GPU);
        layout.addView(mSurface, new LayoutParams(LayoutParams.FILL_PARENT, LayoutParams.FILL_PARENT));
        setContentView(layout, new LayoutParams(LayoutParams.FILL_PARENT, LayoutParams.FILL_PARENT));
        mSurface.requestFocus();
    }

    private void enterImmersiveMode() {
        getWindow().getDecorView().setSystemUiVisibility(
                View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
                        | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                        | View.SYSTEM_UI_FLAG_FULLSCREEN
                        | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                        | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                        | View.SYSTEM_UI_FLAG_LAYOUT_STABLE);
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);
        if (hasFocus) enterImmersiveMode();
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        StrictMode.setThreadPolicy(new StrictMode.ThreadPolicy.Builder().permitAll().build());
        setContentView(R.layout.main);
        enterImmersiveMode();
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
            getWindow().getAttributes().layoutInDisplayCutoutMode =
                    WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES;
        }
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        mSingleton = this;
        mContext = getApplicationContext();
        RES_FILENAME = getResourceName();
        StorageOptions.determineStorageOptions(mContext);
        checkStorageLocationPreference();
        prepareOptionMenu(null);
    }

    public void forceResDownload(final File oldRes) {
        final SDLActivity parent = this;
        new AlertDialog.Builder(this)
                .setTitle("Do you want to download latest core file?")
                .setPositiveButton("Yes", new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int which) {
                        FrameLayout layout = new FrameLayout(parent);
                        setContentView(layout, new LayoutParams(LayoutParams.FILL_PARENT, LayoutParams.FILL_PARENT));
                        oldRes.delete();
                        startDownload();
                    }
                })
                .setNegativeButton("No", new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int which) { mainDisplay(); }
                })
                .create().show();
    }

    public void initializeGame() {
        String coreFileLocation = getSystemStorageLocation() + RES_FILENAME;
        File file = new File(coreFileLocation);
        if (file.exists()) {
            forceResDownload(file);
        } else {
            FrameLayout layout = new FrameLayout(this);
            setContentView(layout, new LayoutParams(LayoutParams.FILL_PARENT, LayoutParams.FILL_PARENT));
            startDownload();
        }
    }

    @Override protected void onPause()  { super.onPause();  SDLActivity.nativePause(); }
    @Override protected void onResume() { super.onResume(); SDLActivity.nativeResume(); }
    @Override public void onDestroy()   { super.onDestroy(); mSurface.onDestroy(); }
    @Override public void onConfigurationChanged(Configuration newConfig) { super.onConfigurationChanged(newConfig); }

    protected void processJGEMsg(final String command) {
        if (null == command) return;
        // The in-game Options screen sends "admin" to open the native import / export /
        // download panel (previously reached by a swipe gesture).
        if (command.equals("admin")) {
            runOnUiThread(new Runnable() {
                @Override public void run() { showAdminMenu(); }
            });
        }
    }

    void sendCommand(int command, Object data) {
        Message msg = commandHandler.obtainMessage();
        msg.arg1 = command;
        msg.obj = data;
        commandHandler.sendMessage(msg);
    }

    // =========================================================================
    // OnKeyListener (Activity level — menu key only)
    // =========================================================================

    public boolean onKey(View v, int keyCode, KeyEvent event) {
        if (keyCode == KeyEvent.KEYCODE_MENU) {
            if (KeyEvent.ACTION_DOWN == event.getAction()) { super.onKeyDown(keyCode, event); return true; }
            if (KeyEvent.ACTION_UP   == event.getAction()) { super.onKeyUp(keyCode, event);   return true; }
        }
        return false;
    }

    // =========================================================================
    // Native interface
    // =========================================================================

    public static native String getResourceUrl();
    public static native String getResourceName();
    public static native void nativeInit();
    public static native void nativeQuit();
    public static native void nativePause();
    public static native void nativeResume();
    public static native void onNativeResize(int x, int y, int format);
    public static native void onNativeKeyDown(int keycode);
    public static native void onNativeKeyUp(int keycode);
    public static native void onNativeTouch(int index, int action, float x, float y, float p);
    public static native void onNativeFlickGesture(float xVelocity, float yVelocity);
    public static native void onNativeAccel(float x, float y, float z);
    public static native void nativeRunAudioThread();

    public static String getSystemFolderPath() { return mSingleton.getSystemStorageLocation(); }
    public static String getUserFolderPath()   { return mSingleton.getUserStorageLocation(); }
    public static void jgeSendCommand(String command) { mSingleton.sendCommand(COMMAND_JGE_MSG, command); }
    public static boolean createGLContext(int majorVersion, int minorVersion) { return mSurface.initEGL(majorVersion, minorVersion); }
    public static void flipBuffers() { mSurface.flipEGL(); }
    public static void setActivityTitle(String title) { mSingleton.sendCommand(COMMAND_CHANGE_TITLE, title); }

    public static Object audioInit(int sampleRate, boolean is16Bit, boolean isStereo, int desiredFrames) {
        int channelConfig = isStereo ? AudioFormat.CHANNEL_CONFIGURATION_STEREO : AudioFormat.CHANNEL_CONFIGURATION_MONO;
        int audioFormat   = is16Bit  ? AudioFormat.ENCODING_PCM_16BIT : AudioFormat.ENCODING_PCM_8BIT;
        int frameSize     = (isStereo ? 2 : 1) * (is16Bit ? 2 : 1);
        desiredFrames = Math.max(desiredFrames,
                ((AudioTrack.getMinBufferSize(sampleRate, channelConfig, audioFormat) + frameSize) - 1) / frameSize);
        mAudioTrack = new AudioTrack(AudioManager.STREAM_MUSIC, sampleRate,
                channelConfig, audioFormat, desiredFrames * frameSize, AudioTrack.MODE_STREAM);
        audioStartThread();
        buf = is16Bit ? new short[desiredFrames * (isStereo ? 2 : 1)] : new byte[desiredFrames * (isStereo ? 2 : 1)];
        return buf;
    }

    public static void audioStartThread() {
        mAudioThread = new Thread(new Runnable() {
            public void run() { mAudioTrack.play(); nativeRunAudioThread(); }
        });
        mAudioThread.setPriority(Thread.MAX_PRIORITY);
        mAudioThread.start();
    }

    public static void audioWriteShortBuffer(short[] buffer) {
        for (int i = 0; i < buffer.length;) {
            int result = mAudioTrack.write(buffer, i, buffer.length - i);
            if (result > 0) i += result;
            else if (result == 0) { try { Thread.sleep(1); } catch (InterruptedException e) {} }
            else { Log.w(TAG, "SDL audio: error return from write(short)"); return; }
        }
    }

    public static void audioWriteByteBuffer(byte[] buffer) {
        for (int i = 0; i < buffer.length;) {
            int result = mAudioTrack.write(buffer, i, buffer.length - i);
            if (result > 0) i += result;
            else if (result == 0) { try { Thread.sleep(1); } catch (InterruptedException e) {} }
            else { Log.w(TAG, "SDL audio: error return from write(byte)"); return; }
        }
    }

    public static void audioQuit() {
        if (mAudioThread != null) {
            try { mAudioThread.join(); } catch (Exception e) { Log.e(TAG, "Problem stopping audio thread: " + e); }
            mAudioThread = null;
        }
        if (mAudioTrack != null) { mAudioTrack.stop(); mAudioTrack = null; }
    }

    // =========================================================================
    // Async download
    // =========================================================================

    class DownloadFileAsync extends AsyncTask<String, Integer, Long> {
        private final String TAG = DownloadFileAsync.class.getCanonicalName();

        @Override
        protected void onPreExecute() {
            super.onPreExecute();
            showDialog(DIALOG_DOWNLOAD_PROGRESS);
        }

        @Override
        protected Long doInBackground(String... aurl) {
            long totalBytes = 0;
            try {
                File resDirectory  = new File(mSingleton.getSystemStorageLocation());
                File userDirectory = new File(mSingleton.getUserStorageLocation());
                if ((!resDirectory.exists() && !resDirectory.mkdirs()) ||
                        (!userDirectory.exists() && !userDirectory.mkdirs())) {
                    throw new Exception("Failed to initialize system and user directories.");
                }
                URL url = new URL(aurl[0]);
                String filename = url.getPath().substring(url.getPath().lastIndexOf('/') + 1);
                URLConnection conexion = url.openConnection();
                conexion.connect();
                int lengthOfFile = conexion.getContentLength();
                InputStream input = new BufferedInputStream(url.openStream());
                OutputStream output = new FileOutputStream(new File(resDirectory, filename));
                byte[] data = new byte[1024];
                int count;
                while ((count = input.read(data)) != -1) {
                    totalBytes += count;
                    publishProgress((int) ((totalBytes * 100) / lengthOfFile));
                    output.write(data, 0, count);
                }
                output.flush();
                output.close();
                input.close();
            } catch (Exception e) {
                mSingleton.downloadError("An error happened while downloading the resources. Please check your connection and try again.");
                Log.e(TAG, e.getMessage());
            }
            return totalBytes;
        }

        protected void onProgressUpdate(Integer... progress) {
            if (progress[0] != mProgressDialog.getProgress()) mProgressDialog.setProgress(progress[0]);
        }

        @Override
        protected void onPostExecute(Long unused) {
            if (mErrorHappened) {
                dismissDialog(DIALOG_DOWNLOAD_PROGRESS);
                showDialog(DIALOG_DOWNLOAD_ERROR);
                return;
            }
            String storageLocation = getSystemStorageLocation();
            File preFile  = new File(storageLocation + RES_FILENAME + ".tmp");
            File postFile = new File(storageLocation + RES_FILENAME);
            if (preFile.exists()) preFile.renameTo(postFile);
            dismissDialog(DIALOG_DOWNLOAD_PROGRESS);
            mSingleton.mainDisplay();
        }
    }
}


// =============================================================================
// SDLSurface
// =============================================================================

class SDLSurface extends SurfaceView implements SurfaceHolder.Callback,
        View.OnKeyListener, View.OnTouchListener, SensorEventListener {
    private static final String TAG = SDLSurface.class.getCanonicalName();

    private static SensorManager mSensorManager;
    private static VelocityTracker mVelocityTracker;
    private static SDLActivity parent;

    private Thread mSDLThread;

    private EGLContext mEGLContext;
    private EGLSurface mEGLSurface;
    private EGLDisplay mEGLDisplay;
    private EGLConfig  mEGLConfig;
    final private Object mSemSurface;
    private Boolean mSurfaceValid;

    public float y1;
    public float y2;
    public final int DELTA_Y = 800;

    public SDLSurface(Context context, SDLActivity app) {
        super(context);
        mSemSurface   = new Object();
        mSurfaceValid = false;
        getHolder().addCallback(this);
        setFocusable(true);
        setFocusableInTouchMode(true);
        requestFocus();
        setOnKeyListener(this);
        setOnTouchListener(this);
        mSensorManager = (SensorManager) context.getSystemService("sensor");
        parent = app;
    }

    void startSDLThread() {
        if (mSDLThread == null) {
            mSDLThread = new Thread(new SDLMain(), "SDLThread");
            mSDLThread.start();
        }
    }

    public void surfaceCreated(SurfaceHolder holder) {
        enableSensor(Sensor.TYPE_ACCELEROMETER, true);
    }

    public void onDestroy() {
        SDLActivity.nativeQuit();
        if (mSDLThread != null) {
            try { mSDLThread.join(); } catch (Exception e) { Log.e(TAG, "Problem stopping thread: " + e); }
            mSDLThread = null;
        }
    }

    public void surfaceDestroyed(SurfaceHolder holder) {
        Log.d(TAG, "surfaceDestroyed()");
        synchronized (mSemSurface) {
            mSurfaceValid = false;
            mSemSurface.notifyAll();
        }
        enableSensor(Sensor.TYPE_ACCELEROMETER, false);
    }

    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        Log.d(TAG, "surfaceChanged()");
        int sdlFormat = 0x85151002;
        switch (format) {
            case PixelFormat.RGBA_4444: sdlFormat = 0x85421002; break;
            case PixelFormat.RGBA_5551: sdlFormat = 0x85441002; break;
            case PixelFormat.RGBA_8888: sdlFormat = 0x86462004; break;
            case PixelFormat.RGBX_8888: sdlFormat = 0x86262004; break;
            case PixelFormat.RGB_332:   sdlFormat = 0x84110801; break;
            case PixelFormat.RGB_565:   sdlFormat = 0x85151002; break;
            case PixelFormat.RGB_888:   sdlFormat = 0x86161804; break;
            default: break;
        }
        SDLActivity.onNativeResize(width, height, sdlFormat);
        startSDLThread();
    }

    public void onDraw(Canvas canvas) {}

    public boolean initEGL(int majorVersion, int minorVersion) {
        Log.d(TAG, "Starting up OpenGL ES " + majorVersion + "." + minorVersion);
        try {
            EGL10 egl = (EGL10) EGLContext.getEGL();
            EGLDisplay dpy = egl.eglGetDisplay(EGL10.EGL_DEFAULT_DISPLAY);
            egl.eglInitialize(dpy, new int[2]);
            int renderableType = (majorVersion == 2) ? 4 : 1;
            int[] configSpec = {EGL10.EGL_RENDERABLE_TYPE, renderableType, EGL10.EGL_NONE};
            EGLConfig[] configs = new EGLConfig[1];
            int[] num_config = new int[1];
            if (!egl.eglChooseConfig(dpy, configSpec, configs, 1, num_config) || num_config[0] == 0) {
                Log.e(TAG, "No EGL config available"); return false;
            }
            mEGLConfig = configs[0];
            EGLContext ctx = egl.eglCreateContext(dpy, mEGLConfig, EGL10.EGL_NO_CONTEXT, null);
            if (ctx == EGL10.EGL_NO_CONTEXT) { Log.e(TAG, "Couldn't create context"); return false; }
            mEGLContext = ctx;
            mEGLDisplay = dpy;
            if (!createSurface(this.getHolder())) return false;
        } catch (Exception e) {
            Log.e(TAG, e + "");
            for (StackTraceElement s : e.getStackTrace()) Log.e(TAG, s.toString());
        }
        return true;
    }

    public Boolean createSurface(SurfaceHolder holder) {
        EGL10 egl = (EGL10) EGLContext.getEGL();
        if (mEGLSurface != null) {
            egl.eglMakeCurrent(mEGLDisplay, EGL10.EGL_NO_SURFACE, EGL10.EGL_NO_SURFACE, EGL10.EGL_NO_CONTEXT);
            egl.eglDestroySurface(mEGLDisplay, mEGLSurface);
        }
        mEGLSurface = egl.eglCreateWindowSurface(mEGLDisplay, mEGLConfig, holder, null);
        if (mEGLSurface == EGL10.EGL_NO_SURFACE) { Log.e(TAG, "Couldn't create surface"); return false; }
        if (!egl.eglMakeCurrent(mEGLDisplay, mEGLSurface, mEGLSurface, mEGLContext)) {
            Log.e(TAG, "Couldn't make context current"); return false;
        }
        mSurfaceValid = true;
        return true;
    }

    public void flipEGL() {
        if (!mSurfaceValid) createSurface(this.getHolder());
        try {
            EGL10 egl = (EGL10) EGLContext.getEGL();
            egl.eglWaitNative(EGL10.EGL_CORE_NATIVE_ENGINE, null);
            egl.eglWaitGL();
            egl.eglSwapBuffers(mEGLDisplay, mEGLSurface);
        } catch (Exception e) {
            Log.e(TAG, "flipEGL(): " + e);
            for (StackTraceElement s : e.getStackTrace()) Log.e(TAG, s.toString());
        }
    }

    // -------------------------------------------------------------------------
    // Key events — back opens admin menu, everything else goes to native
    // -------------------------------------------------------------------------

    public boolean onKey(View v, int keyCode, KeyEvent event) {
        if (keyCode == KeyEvent.KEYCODE_BACK) {
            // BACK (and the system edge-swipe-back gesture, which arrives as BACK) opens the
            // in-game menu ("Back to main menu"). The gesture sends DOWN and UP almost
            // simultaneously; if both reach the engine in the same frame, its
            // HoldKey+ReleaseKey cancel out and GetButtonClick(JGE_BTN_MENU) never fires.
            // So act once on the UP (gesture/press completion), inject a clean key-down now
            // and DEFER the key-up a few frames so the button is observed as held/clicked.
            if (event.getAction() == KeyEvent.ACTION_UP) {
                final int kc = keyCode;
                SDLActivity.onNativeKeyDown(kc);
                v.postDelayed(new Runnable() {
                    public void run() { SDLActivity.onNativeKeyUp(kc); }
                }, 80);
            }
            return true;
        }
        if (keyCode == KeyEvent.KEYCODE_MENU) return false;
        if (keyCode == KeyEvent.KEYCODE_VOLUME_UP || keyCode == KeyEvent.KEYCODE_VOLUME_DOWN) return false;
        if (event.getAction() == KeyEvent.ACTION_DOWN) { SDLActivity.onNativeKeyDown(keyCode); return true; }
        if (event.getAction() == KeyEvent.ACTION_UP)   { SDLActivity.onNativeKeyUp(keyCode);   return true; }
        return false;
    }

    // -------------------------------------------------------------------------
    // Touch events
    // -------------------------------------------------------------------------

    public boolean onTouch(View v, MotionEvent event) {
        switch (event.getAction()) {
            case MotionEvent.ACTION_DOWN:
            case MotionEvent.ACTION_POINTER_DOWN:
                y1 = event.getY();
                break;
            // The admin / import-export menu used to be opened by a swipe gesture here,
            // which interfered with in-game list scrolling. It now lives in the in-game
            // Options screen (see GameStateOptions "Import / Export / Download Data"),
            // so no swipe is intercepted anymore.
        }

        for (int index = 0; index < event.getPointerCount(); ++index) {
            SDLActivity.onNativeTouch(index, event.getActionMasked(),
                    event.getX(index), event.getY(index), event.getPressure(index));
        }

        if (event.getActionIndex() == 0) {
            if (event.getAction() == MotionEvent.ACTION_DOWN) {
                mVelocityTracker = VelocityTracker.obtain();
                mVelocityTracker.clear();
                mVelocityTracker.addMovement(event);
            } else if (event.getAction() == MotionEvent.ACTION_MOVE) {
                mVelocityTracker.addMovement(event);
            } else if (event.getAction() == MotionEvent.ACTION_UP) {
                mVelocityTracker.addMovement(event);
                mVelocityTracker.computeCurrentVelocity(1000);
                float xVelocity = mVelocityTracker.getXVelocity(0);
                float yVelocity = mVelocityTracker.getYVelocity(0);
                if (Math.abs(xVelocity) > 300 || Math.abs(yVelocity) > 300) {
                    SDLActivity.onNativeFlickGesture(xVelocity, yVelocity);
                }
                mVelocityTracker.recycle();
            }
        }
        return true;
    }

    public void enableSensor(int sensortype, boolean enabled) {
        if (enabled) {
            mSensorManager.registerListener(this,
                    mSensorManager.getDefaultSensor(sensortype),
                    SensorManager.SENSOR_DELAY_GAME, null);
        } else {
            mSensorManager.unregisterListener(this,
                    mSensorManager.getDefaultSensor(sensortype));
        }
    }

    public void onAccuracyChanged(Sensor sensor, int accuracy) {}

    public void onSensorChanged(SensorEvent event) {
        if (event.sensor.getType() == Sensor.TYPE_ACCELEROMETER) {
            SDLActivity.onNativeAccel(event.values[0], event.values[1], event.values[2]);
        }
    }

    class SDLMain implements Runnable {
        public void run() {
            SDLActivity.nativeInit();
            SDLActivity.nativeQuit();
            Log.d(TAG, "SDL thread terminated");
            System.exit(0);
        }
    }
}