
# detectron2 details
# Step 1
Input format == RGB
height, width = original_image.shape[:2]
image = self.aug.get_transform(original_image).apply_image(original_image)
image = torch.as_tensor(image.astype("float32").transpose(2, 0, 1))
inputs = {"image": image, "height": height, "width": width}
predictions = self.model([inputs])[0] ***need more detail***

# self.model
call->	
self.inference(batched_inputs)  ***batched_inputs just the input image, this time just have one
	images = self.preprocess_image(batched_inputs)
    features = self.backbone(images.tensor)
	proposals, _ = self.proposal_generator(images, features, None)
	results, _ = self.roi_heads(images, features, proposals, None)
	In Post-Process
		roi_masks = ROIMasks(results.pred_masks[:, 0, :, :])
		also keypoints

# preprocess_image
   Normalize, pad and batch the input images.
 images = [(x - self.pixel_mean) / self.pixel_std for x in images]
   self.pixel_mean = tensor([[[103.5300, 116.2800, 123.6750]]], device='cuda:0')
   self.pixel_std = tensor([[[1., 1., 1.]]], device='cuda:0')
   images = ImageList.from_tensors(
            images,
            self.backbone.size_divisibility,
            padding_constraints=self.backbone.padding_constraints,
        )

# self.backbone(images.tensor)
   bottom_up_features = self.bottom_up(x)
		stem(x)
		       x = self.conv1(x)
			       conv2d ->norm
			   x = F.relu_(x)
               x = F.max_pool2d(x, kernel_size=3, stride=2, padding=1)
		res1-5
# self.proposal_generator





