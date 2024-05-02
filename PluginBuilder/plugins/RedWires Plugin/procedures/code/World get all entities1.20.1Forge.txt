if (world instanceof ServerLevel _server) {
	for (Entity entityiterator : _server.getAllEntities()) {
		${statement$foreach}
	}
}