A = imread('roughness.png');
FA = im2double(A);
FB = FA.^(2.5);
B = im2uint8(FB);
imwrite(B,'roughness_new.png');