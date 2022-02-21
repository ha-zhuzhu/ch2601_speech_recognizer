<?php
// Uploading multiple files
// http://php.net/manual/en/function.move-uploaded-file.php#refsect1-function.move-uploaded-file-examples

$uploads_dir = 'uploads';
//$test=file_get_contents("php://input");
//error_log(print_r($_POST,true));
//error_log(print_r($_FILES,true));
if ($_FILES["file"]["error"] == UPLOAD_ERR_OK)
    {
        $tmp_name = $_FILES["file"]["tmp_name"];
        // basename() may prevent filesystem traversal attacks;
        // further validation/sanitation of the filename may be appropriate
        $name = $_FILES["file"]["name"];
        //$name= 'text1.txt';
        move_uploaded_file($tmp_name, "$uploads_dir/$name");
    }

//var_dump($_FILES);