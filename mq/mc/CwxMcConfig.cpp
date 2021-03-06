#include "CwxMcConfig.h"
#include "CwxMcApp.h"

int CwxMcConfig::loadConfig(string const & strConfFile) {
  CwxIniParse cnf;
  string value;
  string strErrMsg;
  //解析配置文件
  if (false == cnf.load(strConfFile)) {
    CwxCommon::snprintf(m_szErrMsg, 2047,
      "Failure to Load conf file:%s. err:%s", strConfFile.c_str(),
      cnf.getErrMsg());
    return -1;
  }

  //load cmn:home
  if (!cnf.getAttr("cmn", "home", value) || !value.length()) {
    snprintf(m_szErrMsg, 2047, "Must set [cmn:home].");
    return -1;
  }
  if ('/' != value[value.length() - 1])  value += "/";
  m_common.m_strWorkDir = value;
  //load cmn:monitor
  if (!cnf.getAttr("cmn", "monitor", value) || !value.length()) {
    m_common.m_monitor.reset();
  } else {
    if (!mqParseHostPort(value, m_common.m_monitor)) {
      snprintf(m_szErrMsg, 2047, "cmn:monitor must be [host:port], [%s] is invalid.", value.c_str());
      return -1;
    }
  }

  //load store:path
  if (!cnf.getAttr("store", "path", value) || !value.length()) {
    snprintf(m_szErrMsg, 2047, "Must set [store:path].");
    return -1;
  }
  if ('/' != value[value.length() - 1]) value += "/";
  m_store.m_strPath = value;

  //load store:file_max_mbyte
  if (!cnf.getAttr("store", "file_max_mbyte", value) || !value.length()) {
    snprintf(m_szErrMsg, 2047, "Must set [store:file_max_mbyte].");
    return -1;
  }
  m_store.m_uiLogMSize = strtoul(value.c_str(), NULL, 10);
  if (m_store.m_uiLogMSize < CwxMcConfigStore::MIN_LOG_MSIZE) {
    m_store.m_uiLogMSize = CwxMcConfigStore::MIN_LOG_MSIZE;
  }
  if (m_store.m_uiLogMSize > CwxMcConfigStore::MAX_LOG_MSIZE) {
    m_store.m_uiLogMSize = CwxMcConfigStore::MAX_LOG_MSIZE;
  }
  //load store:reserve_day
  if (!cnf.getAttr("store", "reserve_day", value) || !value.length()) {
    snprintf(m_szErrMsg, 2047, "Must set [store:reserve_day].");
    return -1;
  }
  m_store.m_uiReserveDay = strtoul(value.c_str(), NULL, 10);
  //load store:append_return
  if (!cnf.getAttr("store", "append_return", value) || !value.length()) {
    snprintf(m_szErrMsg, 2047, "Must set [store:append_return].");
    return -1;
  }
  if (value=="yes"){
    m_store.m_bAppendReturn = true;
  }else if (value=="no"){
    m_store.m_bAppendReturn = false;
  }else{
    snprintf(m_szErrMsg, 2047, "Invalid [store:append_return]'s value[%s], must be yes/no.", value.c_str());
    return -1;
  }
  //load store:record_prefix
  if (!cnf.getAttr("store", "record_prefix", value) || !value.length()) {
    m_store.m_strRecordPrefix.erase();
  }else{
    m_store.m_strRecordPrefix = value;
  }

  //load store:file_max_second
  if (!cnf.getAttr("store", "file_max_second", value) || !value.length()) {
    snprintf(m_szErrMsg, 2047, "Must set [store:file_max_second].");
    return -1;
  }
  m_store.m_uiSwitchSecond = strtoul(value.c_str(), NULL, 10);
  //load store:flush_log_num
  if (!cnf.getAttr("store", "flush_log_num", value) || !value.length()) {
    snprintf(m_szErrMsg, 2047, "Must set [store:flush_log_num].");
    return -1;
  }
  m_store.m_uiFlushNum = strtoul(value.c_str(), NULL, 10);
  if (m_store.m_uiFlushNum < 1) {
    m_store.m_uiFlushNum = 1;
  }
  //load store:flush_log_second
  if (!cnf.getAttr("store", "flush_log_second", value) || !value.length()) {
    snprintf(m_szErrMsg, 2047, "Must set [store:flush_log_second].");
    return -1;
  }
  m_store.m_uiFlushSecond = strtoul(value.c_str(), NULL, 10);
  if (m_store.m_uiFlushSecond < 1) {
    m_store.m_uiFlushSecond = 1;
  }
  //fetch mq
  if (!fetchHost(cnf, "mq", m_mq.m_mq)) return -1;
  //load mq:log_path
  if (!cnf.getAttr("mq", "name", value) || !value.length()) {
    snprintf(m_szErrMsg, 2047, "Must set [mq:name].");
    return -1;
  }
  m_mq.m_strName = value;
  //load mq:cache_msize
  if (!cnf.getAttr("mq", "cache_msize", value) || !value.length()) {
    snprintf(m_szErrMsg, 2047, "Must set [mq:cache_msize].");
    return -1;
  }
  m_mq.m_uiCacheMSize = strtoul(value.c_str(), NULL, 10);
  if (m_mq.m_uiCacheMSize < 1) {
    m_mq.m_uiCacheMSize = 1;
  }
  //load mq:cache_second
  if (!cnf.getAttr("mq", "cache_second", value) || !value.length()) {
    snprintf(m_szErrMsg, 2047, "Must set [mq:cache_second].");
    return -1;
  }
  m_mq.m_uiCacheTimeout = strtoul(value.c_str(), NULL, 10);
  //load sync
  //load sync:source
  if (!cnf.getAttr("sync", "source", value) || !value.length()) {
    snprintf(m_szErrMsg, 2047, "Must set [sync:source].");
    return -1;
  }
  m_sync.m_strSource = value;
  //load sync:sock_buf_kbyte
  if (!cnf.getAttr("sync", "sock_buf_kbyte", value) || !value.length()) {
    snprintf(m_szErrMsg, 2047, "Must set [sync:sock_buf_kbyte].");
    return -1;
  }
  m_sync.m_uiSockBufKByte = strtoul(value.c_str(), NULL, 10);
  //load sync:max_chunk_kbyte
  if (!cnf.getAttr("sync", "max_chunk_kbyte", value) || !value.length()) {
    snprintf(m_szErrMsg, 2047, "Must set [sync:max_chunk_kbyte].");
    return -1;
  }
  m_sync.m_uiChunkKBye = strtoul(value.c_str(), NULL, 10);
  if (m_sync.m_uiChunkKBye > CWX_MQ_MAX_CHUNK_KSIZE)
    m_sync.m_uiChunkKBye = CWX_MQ_MAX_CHUNK_KSIZE;
  //load sync:sync_conn_num
  if (!cnf.getAttr("sync", "sync_conn_num", value) || !value.length()) {
    snprintf(m_szErrMsg, 2047, "Must set [sync:sync_conn_num].");
    return -1;
  }
  m_sync.m_uiConnNum = strtoul(value.c_str(), NULL, 10);
  if (m_sync.m_uiConnNum < 1) m_sync.m_uiConnNum = 1;
  //load sync:zip
  if (!cnf.getAttr("sync", "zip", value) || !value.length()) {
    m_sync.m_bzip = false;
  } else {
    if (value == "yes")
      m_sync.m_bzip = true;
    else
      m_sync.m_bzip = false;
  }
  return 0;
}

//加载sync的主机
int CwxMcConfig::loadSyncHost(string const& strSyncHostFile){
  CwxIniParse cnf;
  string value;
  string strErrMsg;
  m_syncHosts.m_hosts.clear();
  //解析配置文件
  if (false == cnf.load(strSyncHostFile)) {
    CwxCommon::snprintf(m_szErrMsg, 2047,
      "Failure to Load conf file:%s. err:%s", strSyncHostFile.c_str(),
      cnf.getErrMsg());
    return -1;
  }
  //load sync_host
  list<pair<string, string> > hosts;
  if (!cnf.getAttr("host", hosts) || !hosts.size()){
    snprintf(m_szErrMsg, 2047, "Must set [host].");
    return -1;
  }
  list<pair<string, string> >::iterator iter = hosts.begin();
  list<string> items;
  list<string>::iterator item_iter;
  CwxMcConfigHost hostInfo;
  while(iter != hosts.end()){
    CwxCommon::split(iter->second, items, ':');
    if (items.size() != 4){
      snprintf(m_szErrMsg, 2047, "[host:%s]'s value[%s] is invalid, must be [port:user:passwd:limit].",
        iter->first.c_str(),
        iter->second.c_str());
      return -1;
    }
    hostInfo.m_host = iter->first;
    item_iter = items.begin();
    hostInfo.m_port = strtoul(item_iter->c_str(), NULL, 10);
    ++item_iter;
    hostInfo.m_user = *item_iter;
    ++item_iter;
    hostInfo.m_passwd = *item_iter;
    ++item_iter;
    hostInfo.m_limit = strtoul(item_iter->c_str(), NULL, 10);
    if (m_syncHosts.m_hosts.find(hostInfo.m_host) != m_syncHosts.m_hosts.end()) {
      snprintf(m_szErrMsg, 2047, "[host name:%s] is duplicate.", hostInfo.m_host.c_str());
      return -1;
    }
    m_syncHosts.m_hosts[hostInfo.m_host] = hostInfo;
    ++iter;
  }
  return 0;
}

bool CwxMcConfig::fetchHost(CwxIniParse& cnf, string const& node,
                            CwxHostInfo& host)
{
  string value;
  host.reset();
  //get listen
  if (cnf.getAttr(node, "listen", value) && value.length()) {
    if (!mqParseHostPort(value, host)) {
      snprintf(m_szErrMsg, 2047,
        "%s:listen must be [host:port], [%s] is invalid.", node.c_str(),
        value.c_str());
      return false;
    }
  }
  //load user
  if (cnf.getAttr(node, "user", value) && value.length()) {
    host.setUser(value);
  } else {
    host.setUser("");
  }
  //load passwd
  if (cnf.getAttr(node, "passwd", value) && value.length()) {
    host.setPassword(value);
  } else {
    host.setPassword("");
  }
  if (!host.getHostName().length()) {
    CwxCommon::snprintf(m_szErrMsg, 2047, "Must set [%s]'s [listen].",
      node.c_str());
    return false;
  }
  return true;
}

void CwxMcConfig::outputConfig() const {
  CWX_INFO(("\n*****************BEGIN CONFIG*******************"));
  CWX_INFO(("*****************cmn*******************"));
  CWX_INFO(("home=%s", m_common.m_strWorkDir.c_str()));
  CWX_INFO(("monitor=%s:%u", m_common.m_monitor.getHostName().c_str(), m_common.m_monitor.getPort()));
  CWX_INFO(("*****************store*******************"));
  CWX_INFO(("path=%s", m_store.m_strPath.c_str()));
  CWX_INFO(("file_max_mbyte=%u", m_store.m_uiLogMSize));
  CWX_INFO(("file_max_second=%u", m_store.m_uiSwitchSecond));
  CWX_INFO(("append_return=%s", m_store.m_bAppendReturn?"yes":"no"));
  CWX_INFO(("record_prefix=%s", m_store.m_strRecordPrefix.length()?
    m_store.m_strRecordPrefix.c_str():""));
  CWX_INFO(("flush_log_num=%u", m_store.m_uiFlushNum));
  CWX_INFO(("flush_log_second=%u", m_store.m_uiFlushSecond));
  CWX_INFO(("*****************mq*******************"));
  CWX_INFO(("name=%s", m_mq.m_strName.c_str()));
  CWX_INFO(("user=%s", m_mq.m_mq.getUser().c_str()));
  CWX_INFO(("passwd=%s", m_mq.m_mq.getPasswd().c_str()));
  CWX_INFO(("listen=%s:%u",m_mq.m_mq.getHostName().c_str(), m_mq.m_mq.getPort()));
  CWX_INFO(("cache_msize=%u", m_mq.m_uiCacheMSize));
  CWX_INFO(("cache_second=%u", m_mq.m_uiCacheTimeout));
  CWX_INFO(("*****************sync*******************"));
  CWX_INFO(("source=%s", m_sync.m_strSource.c_str()));
  CWX_INFO(("sock_buf_kbyte=%d", m_sync.m_uiSockBufKByte));
  CWX_INFO(("max_chunk_kbyte=%d", m_sync.m_uiChunkKBye));
  CWX_INFO(("sync_conn_num=%d",m_sync.m_uiConnNum));
  CWX_INFO(("zip=%u", m_sync.m_bzip?"yes":"no"));
  CWX_INFO(("*****************END   CONFIG *******************"));
}

//输出host的配置
void CwxMcConfig::outputSyncHost() const{
  CWX_INFO(("*****************begin sync host*******************"));
  map<string, CwxMcConfigHost>::const_iterator iter = m_syncHosts.m_hosts.begin();
  while(iter != m_syncHosts.m_hosts.end()){
    CWX_INFO(("%s=%u:%s:%s:%u",
      iter->first.c_str(),
      iter->second.m_port,
      iter->second.m_user.c_str(),
      iter->second.m_passwd.c_str(),
      iter->second.m_limit));
    ++iter;
  }
  CWX_INFO(("*****************end sync host*******************"));
}
