static void
complete_highlight_groups(const char str[])
{
  const size_t len = strlen(str);
  for(int i = 0; i < MAXNUM_COLOR; ++i)
  {
    if(strncasecmp(str, GROUPS[i], len) == 0)
    {
      vle_compl_add_match(GROUPS[i],
                          GROUPS_DESCR[i]);
    }
  }
  if(strncmp(str, "clear", len) == 0)
  {
    vle_compl_add_match("clear",
                        "clear color rules");
  }
  vle_compl_finish_group();
  vle_compl_add_last_match(str);
}
