import cv2
import numpy as np
import matplotlib.pyplot as plt


def f(d, inp, oup):
    img1 = cv2.imread('../fuli/' + inp + '1.jpg', 0)
    img2 = cv2.imread('../fuli/' + inp + '2.jpg', 0)

    MIN_MATCH_COUNT = 10

    # find the keypoints and descriptors with SIFT
    kp1, des1 = d.detectAndCompute(img1,None)
    kp2, des2 = d.detectAndCompute(img2,None)

    bf = cv2.BFMatcher(crossCheck = True)
    matches = bf.match(des1, des2)

    # store all the good matches as per Lowe's ratio test.
    good = []
    for m in matches:
        good.append(m)

    src_pts = np.float32([ kp1[m.queryIdx].pt for m in good ]).reshape(-1,1,2)
    dst_pts = np.float32([ kp2[m.trainIdx].pt for m in good ]).reshape(-1,1,2)

    M, mask = cv2.findHomography(src_pts, dst_pts, cv2.RANSAC, 5.0)
    matchesMask = mask.ravel().tolist()


    draw_params = dict(matchColor = (0,255,0), # draw matches in green color
                    matchesMask = matchesMask, # draw only inliers
                    flags = 2)


    img3 = cv2.drawMatches(img1,kp1,img2,kp2,good,None,**draw_params)

    cv2.imwrite(oup + '.png', img3)

    # plt.imshow(img3)
    # plt.show()

for inp in ['object', 'indoor', 'outdoor']:
    f(cv2.xfeatures2d.SIFT_create(), inp, inp + '-m-sift-ransac')
    f(cv2.xfeatures2d.SURF_create(), inp, inp + '-m-surf-ransac')
    f(cv2.ORB_create(), inp, inp + '-m-orb-ransac')
