CREATE TABLE accounts (
  uuid            text primary key,
  username        text,
  server          text,
  password        text,
  display_name    text,
  status          number,
  status_message  text,
  rosterversion   text,
  options         number,
  avatar          text,
  keys            text,
  hostname        text,
  port            number,
  resource        text
);
CREATE TABLE conversations (
  uuid            text,
  accountUuid     text,
  name            text,
  contactUuid     text,
  contactJid      text,
  created         number,
  status          number,
  mode            number,
  attributes      text
);
CREATE TABLE messages (
  uuid                text,
  conversationUuid    text,
  timeSent            number,
  counterpart         text,
  trueCounterpart     text,
  body                text,
  encryption          number,
  status              number,
  type                number,
  relativeFilePath    text,
  serverMsgId         text,
  axolotl_fingerprint text,
  carbon              number,
  edited              number,
  read                number,
  oob                 number,
  errorMsg            text,
  readByMarkers       text,
  markable            number,
  remoteMsgId         text,
  deleted             number,
  bodyLanguage        text
);
CREATE TABLE prekeys (
  account         text,
  id              text,
  key             text
);
CREATE TABLE signed_prekeys (
  account         text,
  id              text,
  key             text
);
CREATE TABLE sessions (
  account         text,
  name            text,
  device_id       text,
  key             text
);
CREATE TABLE identities (
  account         text,
  name            text,
  ownkey          text,
  fingerprint     text,
  certificate     text,
  trust           number,
  active          number,
  last_activation number,
  key             text
);
