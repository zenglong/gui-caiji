<?php
function toUTF8($str)
{
	if(is_array($str)===true)
	{
		foreach ($str as $key => $value)
		{
			$str[$key] = toUTF8($value);
		}
		return $str;
	}
    $encoding = mb_detect_encoding($str, array(
        'ASCII',
    	'GB2312',
    	'GBK',
        'UTF-8',
        'BIG5'
    ));
    return mb_convert_encoding($str, 'UTF-8', $encoding);
}

@set_time_limit(0);
require 'common.inc.php';
require DT_ROOT . '/include/post.func.php';

if ($CFG['charset'] == 'utf-8') {
    foreach ($post as $key => $value) {
        $post[$key] = toUTF8($value);
    }
    foreach ($member as $key => $value) {
        $member[$key] = toUTF8($value);
    }
}

function replace_str_num($num, $content)
{
    if (!preg_match_all("/<img[^>]+>/i", $content, $matches))
        return $content;
    $oldpath = $newpath = array();
    foreach ($matches[0] as $k => $url) {
        $oldpath[] = $url;
        if ($k < $num)
            $newpath[] = $url;
        else
            $newpath[] = '';
    }
    unset($matches);
    return str_replace($oldpath, $newpath, $content);
}

if ($post['maxcaiji_num'] != '' && is_numeric($post['maxcaiji_num']) && $post['maxcaiji_num'] >= 0)
    $post['content'] = replace_str_num($post['maxcaiji_num'], $post['content']); //根据需要采集多少张图片，其他的去掉。
$post['content'] = preg_replace('/style[\S]*?=[\S]*?"[\s\S]*?"/i', '', $post['content']);
$post['content'] = preg_replace('/width[\S]*?=[\S]*?"[\s\S]*?"/i', '', $post['content']);
$post['content'] = preg_replace('/height[\S]*?=[\S]*?"[\s\S]*?"/i', '', $post['content']);
$post['content'] = preg_replace('/[\r\n\t]/i', '', $post['content']);
$post['content'] = preg_replace('/<h[\d]+>/i', '', $post['content']);
$post['content'] = preg_replace('/<\/h[\d]+>/i', '', $post['content']);
$post['content'] = preg_replace('/(<br.*?>)+/i', '<br>', $post['content']);
$post['content'] = str_replace('<p >&nbsp;</p>', '', $post['content']);
$post['content'] = str_replace('<p>&nbsp;</p>', '', $post['content']);
$post['content'] = str_replace('<p></p>', '', $post['content']);
if(DT_VERSION == '5.0' && $post['_zltable_name'] == 'sell')
{
	unset($post['model']);
	unset($post['standard']);
}
if(isset($post['_zltable_name']))
	unset($post['_zltable_name']);

if ($remote !== '') {
    $uploaddir = 'file/upload/' . timetodate($DT_TIME, $DT['uploaddir']) . '/';
    is_dir(DT_ROOT . '/' . $uploaddir) or dir_create(DT_ROOT . '/' . $uploaddir);
    if ($MG['uploadtype'])
        $DT['uploadtype'] = $MG['uploadtype'];
    if ($MG['uploadsize'])
        $DT['uploadsize'] = $MG['uploadsize'];
    if ($remote && strlen($remote) > 17 && strpos($remote, '://') !== false) {
        require DT_ROOT . '/include/remote.class.php';
        $do = new remote($remote, $uploaddir);
    } else {
        require DT_ROOT . '/include/upload.class.php';
        $do = new upload($_FILES, $uploaddir);
    }
    if ($do->save()) {
        $session = new dsession();
        $limit   = intval($MG['uploadlimit']);
        $total   = isset($_SESSION['uploads']) ? count($_SESSION['uploads']) : 0;
        if ($limit && $total > $limit - 1) {
            file_del(DT_ROOT . '/' . $do->saveto);
        }
        if (in_array(strtolower($do->ext), array(
            'jpg',
            'jpeg',
            'gif',
            'png',
            'swf',
            'bmp'
        )) && !@getimagesize(DT_ROOT . '/' . $do->saveto)) {
            file_del(DT_ROOT . '/' . $do->saveto);
        }
        $img_w = $img_h = 0;
        if ($do->image) {
            require DT_ROOT . '/include/image.class.php';
            if (strtolower($do->ext) == 'gif' && in_array($from, array(
                'thumb',
                'album',
                'photo'
            ))) {
                if (!function_exists('imagegif') || !function_exists('imagecreatefromgif')) {
                    file_del(DT_ROOT . '/' . $do->saveto);
                }
            }
            if ($DT['bmp_jpg'] && $do->ext == 'bmp') {
                require DT_ROOT . '/include/bmp.func.php';
                $bmp_src = DT_ROOT . '/' . $do->saveto;
                $bmp     = imagecreatefrombmp($bmp_src);
                if ($bmp) {
                    $do->saveto = str_replace('.bmp', '.jpg', $do->saveto);
                    $do->ext    = 'jpg';
                    imagejpeg($bmp, DT_ROOT . '/' . $do->saveto);
                    file_del($bmp_src);
                    if (DT_CHMOD)
                        @chmod(DT_ROOT . '/' . $do->saveto, DT_CHMOD);
                }
            }
            $info  = getimagesize(DT_ROOT . '/' . $do->saveto);
            $img_w = $info[0];
            $img_h = $info[1];
            if ($DT['max_image'] && in_array($from, array(
                'editor',
                'album',
                'photo'
            ))) {
                if ($img_w > $DT['max_image']) {
                    $img_h = intval($DT['max_image'] * $img_h / $img_w);
                    $img_w = $DT['max_image'];
                    $image = new image(DT_ROOT . '/' . $do->saveto);
                    $image->thumb($img_w, $img_h);
                }
            }
            if ($from == 'thumb') {
            	if($width == '' && $height == '')
            	{
            		$width = $MOD['thumb_width']; //没有提供宽高，则使用模块设置里的默认缩略图宽高值
            		$height = $MOD['thumb_height'];
            	}
                if ($width && $height) {
                    $image = new image(DT_ROOT . '/' . $do->saveto);
                    $image->thumb($width, $height, $DT['thumb_title']);
                    $img_w         = $width;
                    $img_h         = $height;
                    $do->file_size = filesize(DT_ROOT . '/' . $do->saveto);
                }
            } else if ($from == 'album' || $from == 'photo') {
            	if($width == '' && $height == '')
            	{
            		$width = $MOD['thumb_width']; //没有提供宽高，则使用模块设置里的默认缩略图宽高值
            		$height = $MOD['thumb_height'];
            	}
                $saveto     = $do->saveto;
                $do->saveto = $do->saveto . '.thumb.' . $do->ext;
                file_copy(DT_ROOT . '/' . $saveto, DT_ROOT . '/' . $do->saveto);
                $middle = $saveto . '.middle.' . $do->ext;
                file_copy(DT_ROOT . '/' . $saveto, DT_ROOT . '/' . $middle);
                if ($DT['water_type'] == 2) {
                    $image = new image(DT_ROOT . '/' . $saveto);
                    $image->waterimage();
                } else if ($DT['water_type'] == 1) {
                    $image = new image(DT_ROOT . '/' . $saveto);
                    $image->watertext();
                }
                if ($DT['water_type'] && $DT['water_com'] && $_groupid > 5) {
                    $image       = new image(DT_ROOT . '/' . $saveto);
                    $image->text = $_company;
                    $image->pos  = 5;
                    $image->watertext();
                }
                if ($from == 'photo')
                    $DT['thumb_album'] = 0;
                $image = new image(DT_ROOT . '/' . $do->saveto);
                $image->thumb($width, $height, $DT['thumb_album']);
                $image = new image(DT_ROOT . '/' . $middle);
                $image->thumb($DT['middle_w'], $DT['middle_h'], $DT['thumb_album']);
                if ($DT['water_middle'] && $DT['water_type']) {
                    if ($DT['water_type'] == 2) {
                        $image = new image(DT_ROOT . '/' . $middle);
                        $image->waterimage();
                    } else if ($DT['water_type'] == 1) {
                        $image = new image(DT_ROOT . '/' . $middle);
                        $image->watertext();
                    }
                }
            } else if ($from == 'editor') {
                if ($_groupid == 1 && !isset($watermark))
                    $DT['water_type'] = 0;
                if ($DT['water_type']) {
                    $image = new image(DT_ROOT . '/' . $do->saveto);
                    if ($DT['water_type'] == 2) {
                        $image->waterimage();
                    } else if ($DT['water_type'] == 1) {
                        $image->watertext();
                    }
                }
            }
        }
    }
    if ($do->saveto) {
        $member['thumb'] = $CFG['url'] . $do->saveto;
        $post['thumb']   = $CFG['url'] . $do->saveto;
    }
}

if($post['_zlpost_pic'] == 'yes') //如果是单纯的上传图片，则返回上传后的服务端图片地址
{
	exit($CFG['url'] . $do->saveto);
}

//var_dump($post); //调试
//exit(); //调试

define('DT_ADMIN', true);
define('DT_MEMBER', true);
require DT_ROOT . '/admin/global.func.php';
require_once DT_ROOT . '/include/cache.func.php';
isset($file) or $file = 'index';
$_userid  = '1';
$_founder = $CFG['founderid'] == $_userid ? $_userid : 0;
if ($DT['admin_online'])
    admin_online();
$psize = isset($psize) ? intval($psize) : 0;
if ($psize > 0 && $psize != $pagesize) {
    $pagesize = $psize;
    $offset   = ($page - 1) * $pagesize;
}
if ($module == 'destoon') {
    (include DT_ROOT . '/admin/' . $file . '.inc.php') or msg();
} else {
    include DT_ROOT . '/module/' . $module . '/common.inc.php';
    (include MD_ROOT . '/admin/' . $file . '.inc.php') or msg();
}
?> 