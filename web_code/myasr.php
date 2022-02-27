<?php
// 调用腾讯云 SDK 省略
$uploads_dir = 'ch2601_recordings';

if ($_FILES['file']['error'] == UPLOAD_ERR_OK)
{
	$tmp_name = $_FILES['file']['tmp_name'];
	// $name = $_FILES['file']['name'];
	$date_str=date('YmdHis');
	move_uploaded_file($tmp_name, "$uploads_dir/$date_str".'-b2.pcm');
    // 用 ffmpeg 转码，先转大小端并压缩为 1 通道，再转为 wav 格式
	exec('ffmpeg -f s16be -ar 8000 -ac 2 -i '."$uploads_dir/$date_str".'-b2.pcm'.' -f s16le -ar 8000 -ac 1 '."$uploads_dir/$date_str".'-l1.pcm');
	exec('ffmpeg -f s16le -ar 8000 -ac 1 -i '."$uploads_dir/$date_str".'-l1.pcm '."$uploads_dir/$date_str".'-l1.wav');

	try {
        // api 调用部分，省略……
		$resp = $client->SentenceRecognition($req);

		$result_str=$resp->getResult();
		$result_file=fopen("$uploads_dir/$date_str".'.txt',"a");
		fwrite($result_file,$result_str);
		fclose($result_file);
		echo $result_str;
	}
	catch(TencentCloudSDKException $e) {
		echo $e;
	}
}

