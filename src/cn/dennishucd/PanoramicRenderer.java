package cn.dennishucd;

import android.graphics.Bitmap;
import android.opengl.*;
import android.os.SystemClock;
import android.provider.Browser;
import android.util.Log;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;
import java.nio.ByteBuffer;
import java.util.Formatter;
import java.util.concurrent.LinkedBlockingQueue;

/**
 * Created by IntelliJ IDEA.
 * User: 霍峰
 * Date: 11-5-3
 * Time: 上午9:41
 * Description：全景浏览渲染组件
 */
public class PanoramicRenderer implements GLSurfaceView.Renderer {
    private GL10 _gl;
    private Grid mGrid = null;
    private int mTextureID;
    private float mTexWidth;
    private float mTexHeight;
    private float viewWidth;
    private float viewHeight;
    private float viewAspect;

    private float viewYZAngle = 60;
    private float viewXZAngle;
    private float maxViewYZAngle = 60;

    public float yawAngle = 90;   //偏航角 0°~360°
    public float pitchAngle = 0; //俯仰角-90° ~ 90°
    public float scaleSpeed = -30f / 1000f;  //缩放速度
    private float pitchRange;     //俯仰角范围

    private boolean isTouchDown = false;
    float yawSpeed;
    float pitchSpeed;

    private LinkedBlockingQueue<Bitmap> bitmapQueue;

    public PanoramicRenderer(LinkedBlockingQueue<Bitmap> bitmapQueue) {
        this.bitmapQueue = bitmapQueue;

    }

    @Override
    public void onSurfaceCreated(GL10 gl, EGLConfig eglConfig) {
        gl.glDisable(GL10.GL_DITHER);
        gl.glHint(GL10.GL_PERSPECTIVE_CORRECTION_HINT,
                GL10.GL_FASTEST);
        gl.glClearColor(1, 1, 1, 1);
        gl.glEnable(GL10.GL_CULL_FACE);
        gl.glShadeModel(GL10.GL_SMOOTH);
        gl.glEnable(GL10.GL_TEXTURE_2D);
        gl.glEnable(GL10.GL_DEPTH_TEST);
        gl.glEnable(GL10.GL_TEXTURE_2D);
        gl.glDisable(GL10.GL_DITHER);

        gl.glClearColor(0, 0, 0, 0);
        gl.glEnable(GL10.GL_DEPTH_TEST);
        gl.glEnable(GL10.GL_CULL_FACE);

        _gl = gl;
//        mGrid = generateSphereGrid();
    }

    @Override
    public void onSurfaceChanged(GL10 gl, int width, int height) {
        gl.glViewport(0, 0, width, height);
        viewAspect = (float) width / height;
        viewHeight = height;
        viewWidth = width;
        setMaxViewYZAngle(maxViewYZAngle);
        setViewYZAngle(maxViewYZAngle);
    }

    @Override
    public void onDrawFrame(GL10 gl) {
        Log.i("lihb test", "onDrawFrame called");

        Bitmap bitmap = bitmapQueue.poll();
        if (bitmap != null) {
            gl.glDeleteTextures(1, new int[]{mTextureID}, 0);

            int[] textures = new int[1];
            gl.glGenTextures(1, textures, 0);
            mTextureID = textures[0];
            Log.i("lihb test----", "poll()-->" + bitmap.toString());
            // 设置纹理贴图
            setTexture(bitmap);
        }else {
            Log.i("lihb test----", "poll()--> bitmap = null");
            return;
        }

        try {
            Thread.sleep(40);
        } catch (InterruptedException e) {
            e.printStackTrace();
        }

        if (mGrid == null) return;

        update();

        gl.glTexEnvx(GL10.GL_TEXTURE_ENV, GL10.GL_TEXTURE_ENV_MODE,
                GL10.GL_MODULATE);
        gl.glClear(GL10.GL_COLOR_BUFFER_BIT | GL10.GL_DEPTH_BUFFER_BIT);

        gl.glMatrixMode(GL10.GL_MODELVIEW);
        gl.glLoadIdentity();

        gl.glRotatef(pitchAngle, 1f, 0f, 0.0f);
        gl.glRotatef(yawAngle, 0.0f, 1f, 0.0f);

        gl.glEnableClientState(GL10.GL_VERTEX_ARRAY);
        gl.glEnableClientState(GL10.GL_TEXTURE_COORD_ARRAY);

        gl.glActiveTexture(GL10.GL_TEXTURE0);
        gl.glBindTexture(GL10.GL_TEXTURE_2D, mTextureID);

        mGrid.draw(gl);
    }

    private void checkGlError(String op) {
        int error;
        while ((error = GLES10.glGetError()) != GLES10.GL_NO_ERROR) {
            Log.e("Panoramic", op + ": glError " + GLU.gluErrorString(error));
        }
    }

    public void setTexture(Bitmap bitmap) {
        if (bitmap == null) return;
        Log.i("lihb test----", "in setTexture()");
        _gl.glBindTexture(GL10.GL_TEXTURE_2D, mTextureID);

        _gl.glTexParameterf(GL10.GL_TEXTURE_2D, GL10.GL_TEXTURE_MIN_FILTER,
                GL10.GL_LINEAR);
        _gl.glTexParameterf(GL10.GL_TEXTURE_2D,
                GL10.GL_TEXTURE_MAG_FILTER,
                GL10.GL_LINEAR);

        _gl.glTexParameterf(GL10.GL_TEXTURE_2D, GL10.GL_TEXTURE_WRAP_S,
                GL10.GL_CLAMP_TO_EDGE);
        _gl.glTexParameterf(GL10.GL_TEXTURE_2D, GL10.GL_TEXTURE_WRAP_T,
                GL10.GL_CLAMP_TO_EDGE);

        _gl.glTexEnvf(GL10.GL_TEXTURE_ENV, GL10.GL_TEXTURE_ENV_MODE,
                GL10.GL_REPLACE);

        mTexWidth = bitmap.getWidth();
        mTexHeight = bitmap.getHeight();


        GLUtils.texImage2D(GL10.GL_TEXTURE_2D, 0, bitmap, 0);
//        int bytes = bitmap.getByteCount();
//        ByteBuffer byteBuffer = ByteBuffer.allocate(bytes);
//        bitmap.copyPixelsToBuffer(byteBuffer);
//        Log.i("lihb test----", "bytes = " + bytes);
//        _gl.glTexImage2D(GL10.GL_TEXTURE_2D, /*level*/ 0, GL10.GL_RGBA,
//                512, 512, /*border*/ 0, GL10.GL_RGBA, GL10.GL_UNSIGNED_BYTE, byteBuffer);
        checkGlError("texImage2D");
        Log.i("lihb test----", "in setTexture() after texImage2D ");

//        mGrid = null;
        mGrid = generateSphereGrid();

    }

    public Grid generateSphereGrid() {
        Log.i("lihb test----", "generateSphereGrid() begin ");
        final int vSteps = 20;
        final int uSteps = (int) (vSteps * (mTexWidth / mTexHeight));
        Grid grid = new Grid(uSteps + 1, vSteps + 1);
        double vAngle, uAngle;
        double uTotalAngle = Math.PI * 2;
        double vTotalAngle = uTotalAngle * (mTexHeight / mTexWidth);
        maxViewYZAngle = (float) (vTotalAngle / Math.PI * 180);

        double vStartAngle = vTotalAngle / 2;
        pitchRange = 90;
        if (vStartAngle < 90) {
            pitchRange = (float) ((vStartAngle * 180 / Math.PI) - viewYZAngle / 2);
        }
        float r = 2.0f;
        float x, y, z, rxz, u, v;
        for (int j = 0; j <= vSteps; j++) {
            vAngle = vStartAngle - vTotalAngle * j / vSteps;
            rxz = Math.abs(r * (float) Math.cos(vAngle));
            y = r * (float) Math.sin(vAngle);
            v = (float) j / vSteps;
            for (int i = uSteps; i >= 0; i--) {
                uAngle = uTotalAngle * i / uSteps;
                x = rxz * (float) Math.cos(uAngle);
                z = rxz * (float) Math.sin(uAngle);
                u = (float) i / uSteps;
                grid.set(i, j, x, y, z, u, v);
            }
        }

        grid.createBufferObjects(_gl);
        Log.i("lihb test----", "generateSphereGrid() over");
        return grid;
    }


    //确保视角不至于过大
    public void setMaxViewYZAngle(float maxYZAngle) {
        final float maxAngel = 90;
        if (maxYZAngle > maxAngel) {
            maxYZAngle = maxAngel;
        }
        float maxXZAngle = maxYZAngle * viewAspect;
        if (maxXZAngle > maxAngel) {
            maxXZAngle = maxAngel;
            maxYZAngle = maxXZAngle / viewAspect;
        }
        this.maxViewYZAngle = maxYZAngle;
    }

    public void setViewYZAngle(float angle) {
        if (angle > maxViewYZAngle) {
            angle = maxViewYZAngle;
        }
        viewYZAngle = angle;
        viewXZAngle = viewYZAngle * viewAspect;
        _gl.glMatrixMode(GL10.GL_PROJECTION);
        _gl.glLoadIdentity();
        GLU.gluPerspective(_gl, viewYZAngle, viewAspect, 0.1f, 10.0f);
    }



    long lastUpdateTime = 0;

    public void update() {
        float delta = 0;
        if (lastUpdateTime > 0)
            delta = SystemClock.uptimeMillis() - lastUpdateTime;
        lastUpdateTime = SystemClock.uptimeMillis();
        if (isTouchDown) return;
        if (pitchSpeed != 0) {
            if (!setPitchAngle(pitchSpeed * delta)) {
                pitchSpeed = 0;
            } else {
                pitchSpeed *= 0.95;
            }
        }

        if (yawSpeed != 0) {
            yawAngle += yawSpeed * delta;
            yawSpeed *= 0.55;
        }
//        if (scaleSpeed != 0) {
//            setViewYZAngle(viewYZAngle + scaleSpeed * delta);
//            scaleSpeed *= 0.9;
//        }
    }

    public boolean setPitchAngle(float angel) {
        pitchAngle += angel;
        if (pitchAngle > pitchRange) {
            pitchAngle = pitchRange;
        } else if (pitchAngle < -pitchRange) {
            pitchAngle = -pitchRange;
        } else {
            return true;
        }
        return false;
    }

    private float lastX, lastY, lastMotionX, lastMotionY;
    private float deltaX, deltaY;
    long lastTouchTime;
    long deltaMotionTime;

    public boolean onTouchDown(float x, float y) {
        lastX = x;
        lastY = y;
        isTouchDown = true;
        lastMotionX = x;
        lastMotionY = y;
        lastTouchTime = SystemClock.uptimeMillis();
        return true;
    }


    public boolean onTouchMove(float x, float y) {
        deltaX = lastX - x;
        deltaY = lastY - y;
        yawAngle += deltaX / viewWidth * viewXZAngle;
        setPitchAngle(deltaY / viewHeight * viewYZAngle);
        lastX = x;
        lastY = y;
        deltaMotionTime = SystemClock.uptimeMillis() - lastTouchTime;
        if (deltaMotionTime >= 50) {
            yawSpeed = (lastMotionX - x) / viewWidth * viewXZAngle / ((float) deltaMotionTime);
            pitchSpeed = (lastMotionY - y) / viewHeight * viewYZAngle / ((float) deltaMotionTime);
            lastTouchTime = SystemClock.uptimeMillis();
            lastMotionX = x;
            lastMotionY = y;
            Log.d("Speed", String.format("yaw:%f, pitch:%f", yawSpeed, pitchSpeed));
        }
        return true;
    }

    public boolean onTouchUp(float x, float y) {
        isTouchDown = false;

        yawSpeed *= 3;
//        pitchSpeed *= 2;
        Log.d("Speed", String.format("Last: yaw:%f, pitch:%f", yawSpeed, pitchSpeed));
        return true;
    }
}
