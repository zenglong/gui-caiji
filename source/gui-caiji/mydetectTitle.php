<?php 
@set_time_limit(0);
@error_reporting(0);
define('IN_DESTOON',true);

if(isset($_POST['_zlmy_action']) && $_POST['_zlmy_action'] == 'getCats')
{
	$ret_array = array();
	$ret_array[] = include 'file/cache/module.php';
	include_once 'common.inc.php';
	$sql="SELECT * FROM {$db->pre}category";
	$query = $db->query($sql);
	while($r = $db->fetch_array($query)) {
		$category[] = $r;
	}
	$ret_array[] = $category;
	exit(serialize($ret_array));
}
elseif(isset($_POST['_zlmy_action']) && $_POST['_zlmy_action'] == 'getCatsXml')
//elseif($_GET['_zlmy_action'] == 'getCatsXml')
{
	$allInfos[] = include 'file/cache/module.php';
	$modules = $allInfos[0]['module'];
	$dt_info = $allInfos[0]['dt'];
	include_once 'common.inc.php';
	ob_end_clean();
	$sql="SELECT * FROM {$db->pre}category order by moduleid asc";
	$query = $db->query($sql);
	echo '<?xml version="1.0"?>'.chr(13).chr(10);
	echo "<webroot name=\"".toGBK($dt_info['sitename'])."\" url=\"".toGBK($dt_info['linkurl'])."\">".chr(13).chr(10);
	ob_flush();
	flush();
	while($category = $db->fetch_array($query)) {
		if(!isset($modules[$category['moduleid']]))
			continue;
		$category['moduleName'] = $modules[$category['moduleid']]['name'];
		$siteid = mysql_escape_string($siteid);
		$module =  mysql_escape_string($category['moduleName']);
		$moduleid = mysql_escape_string($category['moduleid']);
		$name = mysql_escape_string($category['catname']);
		$catid = mysql_escape_string($category['catid']);
		if(isset($category['seo_keywords']) && $category['seo_keywords']!='')
		{
			$keywords = $category['seo_keywords'];
			$keywords = explode(',', str_replace(array('|','£¬'),array(',',','),$keywords));
			foreach ($keywords as $i => $keyword)
			{
				$keywords[$i] = array();
				if(strpos($keyword, '_'))
				{
					$keyword = explode('_', $keyword);
					$keywords[$i]['attr']['must'] = $keyword[1];
					$keyword = $keyword[0];
				}
				else
					$keywords[$i]['attr']['must'] = '';
				$keywords[$i]['name'] = $keyword;
				$keywords[$i]['attr']['needSuffix'] = '1';
			}
		}
		else
		{
			$keywords = array();
		}
		echo chr(9)."<cate name=\"".toGBK($name)."\" catid=\"".toGBK($catid)."\" modid=\"".toGBK($moduleid)."\" >".chr(13).chr(10);
		foreach ($keywords as $keyword)
		{
			echo chr(9).chr(9).'<keyword name="'.toGBK(str_replace(array(chr(13),chr(10),' '),'',strip_tags($keyword['name']))).'" must="'.toGBK($keyword['attr']['must']).'" needSuffix="'.toGBK($keyword['attr']['needSuffix']).'">';
			echo '</keyword>'.chr(13).chr(10);
		}
		echo chr(9)."</cate>".chr(13).chr(10);
		ob_flush();
		flush();
	}
	echo "</webroot>";
	exit();
}
else if(isset($_GET['_zlmy_get_action']) && $_GET['_zlmy_get_action']=='getModules')
{
	include_once 'common.inc.php';
	$ret_array = include 'file/cache/module.php';
	echo '<?xml version="1.0"?>'.chr(13).chr(10);
	echo "<modroot>".chr(13).chr(10);
	foreach ($ret_array['module'] as $i => $m)
	{
		echo chr(9).'<mod module="'.toGBK($m['module']).'" id="'.toGBK($i).'" name="'.toGBK($m['name']).
		'"></mod>'.chr(13).chr(10);
	}
	echo "</modroot>";
	exit();
}
else if(isset($_GET['_zlmy_get_action']) && $_GET['_zlmy_get_action']=='CheckCompanyUser')
{
	include_once 'common.inc.php';
	if($CFG['charset'] == 'utf-8')
	{
		$companyname = toUTF8($_POST['_zlmy_detectCompany']);
	}
	else
	{
		$companyname = $_POST['_zlmy_detectCompany'];
	}
	$sql="select * from {$db->pre}member WHERE company='".mysql_escape_string($companyname)."'";
	$result = $db->get_one($sql);
	if($result===false)
		exit('the company is none!!!');
	else
	{
		exit($result['username']);
	}
}
else if(isset($_GET['_zlmy_get_action']) && $_GET['_zlmy_get_action']=='GetItemId')
{
	include_once 'common.inc.php';
	if($CFG['charset'] == 'utf-8')
	{
		$table = toUTF8($_POST['_zlmy_detectTable']);
		$title = toUTF8($_POST['_zlmy_detectTitle']);
		$catid = toUTF8($_POST['_zlmy_catid']);
		$modid = toUTF8($_POST['_zlmy_modid']);
	}
	else
	{
		$table = $_POST['_zlmy_detectTable'];
		$title = $_POST['_zlmy_detectTitle'];
		$catid = $_POST['_zlmy_catid'];
		$modid = $_POST['_zlmy_modid'];
	}
	if(DT_VERSION == '5.0' &&
			($table == 'sell' ||
					$table == 'buy' ||
					$table == 'brand' ||
					$table == 'down' ||
					$table == 'photo' ||
					$table == 'video'))
	{
		$table = $table . '_' . $modid;
	}
	//$sql="select * from {$db->pre}{$table} WHERE title='".mysql_escape_string($title)."' and catid={$catid}";
	$sql="select * from {$db->pre}{$table} WHERE title='".mysql_escape_string($title)."'";
	$result = $db->get_one($sql);
	if($result===false)
		exit('N');
	else
		exit($result['itemid']);
}
else
{
	include_once 'common.inc.php';
	if($CFG['charset'] == 'utf-8')
	{
		$table = toUTF8($_POST['_zlmy_detectTable']);
		$title = toUTF8($_POST['_zlmy_detectTitle']);
		$catid = toUTF8($_POST['_zlmy_catid']);
		$modid = toUTF8($_POST['_zlmy_modid']);
	}
	else
	{
		$table = $_POST['_zlmy_detectTable'];
		$title = $_POST['_zlmy_detectTitle'];
		$catid = $_POST['_zlmy_catid'];
		$modid = $_POST['_zlmy_modid'];
	}
	if(DT_VERSION == '5.0' && 
		($table == 'sell' || 
		 $table == 'buy' ||
		 $table == 'brand' ||
		 $table == 'down' ||
		 $table == 'photo' ||
		 $table == 'video'))
	{
		$table = $table . '_' . $modid;
	}
	//$sql="select * from {$db->pre}{$table} WHERE title='".mysql_escape_string($title)."' and catid={$catid}";
	$sql="select * from {$db->pre}{$table} WHERE title='".mysql_escape_string($title)."'";
	$result = $db->get_one($sql);
	if($result===false)
		exit('N');
	else
		exit('Y');
}

function toGBK($str){
	global $CFG;
	if($CFG['charset'] == 'utf-8')
		$encoding = 'UTF-8';
	else
		$encoding = 'GBK';
	return mb_convert_encoding($str, 'GBK', $encoding);
}
function toUTF8($str){
	$encoding = mb_detect_encoding($str, array('ASCII','GB2312','GBK','UTF-8','BIG5'));
	return mb_convert_encoding($str, 'UTF-8', $encoding);
}
?>