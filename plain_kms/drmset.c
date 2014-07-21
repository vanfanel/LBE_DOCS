#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>
#include "drm_names.h"
#include "drmset.h"
#include <drm_fourcc.h>
//#include <drm_mode.h>
#define DRM_MODE_OBJECT_PLANE 0xeeeeeeee


struct drm_videodevice *drmDevice;

void macPlane ();


int macDrmSetup (){
	/*¡¡¡CUIDADO!!! Los mensajes informando de conectores, encoders, crtcs, etc.. dispodibles los nombran
	por tipo y no por Id. Esto no está bien y lo hago para simplificar: en realidad, es posible tener más
	de un recurso (un conector, un encoder, un crtc) del mismo tipo, y con distintas IDs, pero no es muy 
	común en equipos "domésticos"
	*/ 

	//FASE1 Abrimos el file descriptor de la tarjeta		
	//Recuerda que pitch = stride
	drmDevice = (drm_videodevice*) malloc (sizeof (drm_videodevice));	
	int ret;
	unsigned int i, j, k;
	uint64_t has_dumb;	

	struct drm_mode_create_dumb creq;
	struct drm_mode_destroy_dumb dreq;
	struct drm_mode_map_dumb mreq;

	const char *card = "/dev/dri/card0";
	drmDevice->fd = open(card, O_RDWR | O_CLOEXEC);
	if (drmDevice->fd < 0) {
		fprintf(stderr, "no se pudo abrir fd de tarjeta '%s': %m\n", card);
		return -1;
	}
	fprintf(stderr, "usando tarjeta '%s'\n", card);

	if (drmGetCap(drmDevice->fd, DRM_CAP_DUMB_BUFFER, &has_dumb) < 0 ||
	    !has_dumb) {
		printf("drm device '%s' does not support dumb buffers\n", card);
		close(drmDevice->fd);
		return -1;
	}



	//FASE 2: a partir del filedescriptor DRM, obtenemos toda la información de sus resources (conectores 
	//físicos, crtcs). Entonces, deberíamos elegir un conector concreto e inicializarlo. 
	//A partir del conector elegido, buscamos el encoder que está usando, y a partir del encoder, el 
	//crtc que está usando ese encoder.
	
	drmDevice->resources = drmModeGetResources(drmDevice->fd);
	if (!drmDevice->resources) {
		printf("ERROR - No se pudo recuperar resources de la tarjeta\n");
		return -1;
	}

	printf ("Número de conectores: %d\n", drmDevice->resources->count_connectors);
	//Recorro la lista de conectores mirando cuál está conectado. El primero que se encuentre
	//conectado, es el que se usa. Por ahora, simplificamos haciendo esto.
	for (i = 0; i < drmDevice->resources->count_connectors; ++i) {
		//Miramos qué conectores tienen monitor conectado: imprimimos por tipo, no por ID. 
		drmDevice->connector = drmModeGetConnector(drmDevice->fd, drmDevice->resources->connectors[i]);
		if (drmDevice->connector->connection == DRM_MODE_CONNECTED) {
			printf("OK. Encontrado conector conectado con ID %d y tipo %s\n",
			drmDevice->connector->connector_id, connector_type_str(drmDevice->connector->connector_type));
			
			//escogemos el primer modo de vídeo disponible del conector, que es el más alto:
			memcpy(&drmDevice->mode, &drmDevice->connector->modes[0], sizeof(drmDevice->mode));
			break;
		}
		//Nótese que al salir del for, conn ya queda elegido como drmDevice->conectore el 
		//primer conector conectado.
	}
	

	printf ("modos de vídeo disponibles:\n");
	for (i = 0; i < drmDevice->connector->count_modes; i++ ){
		printf ("%d x %d\n", drmDevice->connector->modes[i].hdisplay, 
				     drmDevice->connector->modes[i].vdisplay);
	}

	//Buscamos un CRTC para controlar este conector. Un CRTC conecta un scaout buffer con un conector.
	//-Un CRTC puede controlar varios conectores si en todos se va a ver la misma imágen.
	//-Cada CRTC puede trabajar con unos encoders y con otros no. Así que recorremos la lista de encoders
	// y buscamos un CRTC que pueda trabajar con cada encoder.	
	
	//Pillamos el encoder que se está usando, a partir del conector:
	drmDevice->encoder = drmModeGetEncoder(drmDevice->fd, drmDevice->connector->encoder_id);	

	//Pillamos el crtc que se está usando con ese encoder:
	drmDevice->crtc = drmModeGetCrtc (drmDevice->fd, drmDevice->encoder->crtc_id);

/*	printf ("Número de encoders: %d\n", resources->count_encoders);
	for (i = 0; i < resources->count_encoders; i++){
		enc = drmModeGetEncoder (drmDevice->fd, resources->encoders[i]);	
		printf ("Disponemos de encoder tipo: %s\n", encoder_type_str(enc->encoder_type) );
	}
*/

	

	
	//FASE 3: Creamos el buffer.
	//De momento, sólo uno.
	//Reservamos memoria para el buffer. El stride nos lo devuelve calculado la función, no lo conocemos
	//previamente.
	
	/* create dumb buffer */
	memset(&creq, 0, sizeof(creq));
	creq.width = drmDevice->mode.hdisplay;
	creq.height = drmDevice->mode.vdisplay;
	creq.bpp = 32;
	ret = drmIoctl(drmDevice->fd, DRM_IOCTL_MODE_CREATE_DUMB, &creq);
	if (ret < 0) {
		printf("cannot create dumb buffer (%s)\n",strerror(-ret));
		return -1;
	}
	drmDevice->stride = creq.pitch;
	drmDevice->size = creq.size;
	drmDevice->handle = creq.handle;

	uint32_t pixel_format = DRM_FORMAT_XRGB8888;
//	uint32_t pixel_format = DRM_FORMAT_RGB565;
	
	ret = drmModeAddFB(drmDevice->fd, drmDevice->mode.hdisplay, drmDevice->mode.vdisplay, 24, 32, 
		drmDevice->stride, drmDevice->handle, &drmDevice->fb);

	if (ret) {
		printf("ERROR - No se pudo crear objeto framebuffer,%s\n", strerror(errno));
		return (-1);
	}



	//Mapeamos la memoria del buffer object para poder escribir en ella. En double buffer
	//se necesitan dos.

	/* prepare buffer for memory mapping */
	memset(&mreq, 0, sizeof(mreq));
	mreq.handle = drmDevice->handle;
	ret = drmIoctl(drmDevice->fd, DRM_IOCTL_MODE_MAP_DUMB, &mreq);
	if (ret) {
		printf("cannot map dumb buffer (%s)\n", strerror (-errno));
		return (-1);
	}

	/* perform actual memory mapping */
	drmDevice->mappedmem = mmap(0, drmDevice->size, PROT_READ | PROT_WRITE, MAP_SHARED,
		        drmDevice->fd, mreq.offset);
	if (drmDevice->mappedmem == MAP_FAILED) {
		printf("cannot mmap dumb buffer (%s)\n",strerror (-errno));
		return (-1);
	}

	/* clear the framebuffer to 0 */
	memset(drmDevice->mappedmem, 0, drmDevice->size);


	
	//FASE 4: Llevamos a cabo el modesetting propiamente dicho: Ya tenemos un conector y un CRTC, así que 
	//conectamos el CRTC con el framebuffer que visualizaremos.
	/*printf ("MAC INFO : crtc %d , conn  %d, mode xres %d, mode yres %d\n", 
		drmDevice->crtc.crtc_id, drmDevice->connnector.connector_id, 
		drmDevice->mode.hdisplay, drmDevice->mode.vdisplay);
	*/
	ret = drmModeSetCrtc(drmDevice->fd, drmDevice->crtc->crtc_id, drmDevice->fb, 0, 0,
		&(drmDevice->connector->connector_id), 1, &drmDevice->mode);

	if (ret){
		printf ("\nError - No se pudo conectar CRTC elegido con conector y objeto framebuffer, %s\n", 
			strerror(errno));
		return (-1);
	};

	//Esto del fb_end se usa luego para dibujar en el buffer
	//drmDevice->size = (drmDevice->mode.vdisplay * drmDevice->stride);

	//Por ahora los planos no funcionan en el portátil. Mala suerte.
	//macPlane();
	printf ("***KMS init completed\n");	

	return 0;
}

//int macDrmSetupLibKMS (){
	/*drmDevice = (drm_videodevice*) malloc (sizeof (drm_videodevice));	
	unsigned int i, j, k;
	
	char *modules[] = { "i915", "nouveau", "vmwgfx", "radeon", "exynos" };
	for (i = 0; i < (sizeof(modules)/sizeof(modules[0])); i++) {
		drmDevice->fd = drmOpen(modules[i], NULL);
		if (drmDevice->fd >= 0) {
			printf("\nCargado con éxito módulo DRM para %s\n",modules[i]);
			break;
		}
	}	

	if (kms_create(drmDevice->fd, &drmDevice->kms)) {
                printf("Error creando kms driver\n");
                exit(0);
        }	

	//FASE 2: a partir del filedescriptor DRM, obtenemos toda la información de sus resources (conectores 
	//físicos, crtcs). Entonces, deberíamos elegir un conector concreto e inicializarlo. 
	//A partir del conector elegido, buscamos el encoder que está usando, y a partir del encoder, el 
	//crtc que está usando ese encoder.
	
	drmDevice->resources = drmModeGetResources(drmDevice->fd);
	if (!drmDevice->resources) {
		printf("ERROR - No se pudo recuperar resources de la tarjeta\n");
		return -1;
	}

	printf ("Número de conectores: %d\n", drmDevice->resources->count_connectors);
	//Recorro la lista de conectores mirando cuál está conectado. El primero que se encuentre
	//conectado, es el que se usa. Por ahora, simplificamos haciendo esto.
	for (i = 0; i < drmDevice->resources->count_connectors; ++i) {
		//Miramos qué conectores tienen monitor conectado: imprimimos por tipo, no por ID. 
		drmDevice->connector = drmModeGetConnector(drmDevice->fd, drmDevice->resources->connectors[i]);
		if (drmDevice->connector->connection == DRM_MODE_CONNECTED) {
			printf("OK. Encontrado conector conectado con ID %d y tipo %s\n",
			drmDevice->connector->connector_id, connector_type_str(drmDevice->connector->connector_type));
			
			//escogemos el primer modo de vídeo disponible del conector, que es el más alto:
			memcpy(&drmDevice->mode, &drmDevice->connector->modes[0], sizeof(drmDevice->mode));
			break;
		}
		//Nótese que al salir del for, conn ya queda elegido como drmDevice->conectore el 
		//primer conector conectado.
	}
	

	printf ("modos de vídeo disponibles:\n");
	for (i = 0; i < drmDevice->connector->count_modes; i++ ){
		printf ("%d x %d\n", drmDevice->connector->modes[i].hdisplay, 
				     drmDevice->connector->modes[i].vdisplay);
	}

	//Buscamos un CRTC para controlar este conector. Un CRTC conecta un scaout buffer con un conector.
	//-Un CRTC puede controlar varios conectores si en todos se va a ver la misma imágen.
	//-Cada CRTC puede trabajar con unos encoders y con otros no. Así que recorremos la lista de encoders
	// y buscamos un CRTC que pueda trabajar con cada encoder.	
	
	//Pillamos el encoder que se está usando, a partir del conector:
	drmDevice->encoder = drmModeGetEncoder(drmDevice->fd, drmDevice->connector->encoder_id);	

	//Pillamos el crtc que se está usando con ese encoder:
	drmDevice->crtc = drmModeGetCrtc (drmDevice->fd, drmDevice->encoder->crtc_id);

	//FASE 3: Creamos el buffer.
	//De momento, sólo uno.
	//Reservamos memoria para el buffer. El stride nos lo devuelve calculado la función, no lo conocemos
	//previamente.
	drmDevice->bo[0] = drmAllocateBuffer(drmDevice->kms, drmDevice->mode.htotal, 
		drmDevice->mode.vtotal, &drmDevice->stride);
	if (drmDevice->bo[0] == NULL){
		printf ("\nERR - No se pudo crear buffer %d\n", i);
		return (-1);
	}	
	
	int ret;
	
	uint32_t handles[4] = {0,0,0,0};
	uint32_t pitches[4] = {0,0,0,0}; 
	uint32_t offsets[4] = {0,0,0,0};

	kms_bo_get_prop(drmDevice->bo[0], KMS_HANDLE, &handles[0]);
	pitches[0] = drmDevice->stride;
	offsets[0] = 0;

	uint32_t pixel_format = DRM_FORMAT_XRGB8888;
//	uint32_t pixel_format = DRM_FORMAT_RGB565;
	
	ret = drmModeAddFB2(drmDevice->fd, drmDevice->mode.hdisplay, drmDevice->mode.vdisplay,
		pixel_format, handles, pitches, offsets, &drmDevice->fb, 0);	

	if (ret) {
		printf("ERROR - No se pudo crear objeto framebuffer,%s\n", strerror(errno));
		return (-1);
	}

	//Mapeamos la memoria del buffer object para poder escribir en ella. En double buffer
	//se necesitan dos.
	kms_bo_map (drmDevice->bo[0], (void**) &drmDevice->mappedmem);
	
	//FASE 4: Llevamos a cabo el modesetting propiamente dicho: Ya tenemos un conector y un CRTC, así que 
	//conectamos el CRTC con el framebuffer que visualizaremos.
	ret = drmModeSetCrtc(drmDevice->fd, drmDevice->crtc->crtc_id, drmDevice->fb, 0, 0,
		&(drmDevice->connector->connector_id), 1, &drmDevice->mode);

	if (ret){
		printf ("\nError - No se pudo conectar CRTC elegido con conector y objeto framebuffer, %s\n", 
			strerror(errno));
		return (-1);
	};

	//Esto del fb_end se usa luego para dibujar en el buffer
	drmDevice->size = (drmDevice->mode.vdisplay * drmDevice->stride);

	//Por ahora los planos no funcionan en el portátil. Mala suerte.
	//macPlane();

	return 0;
	*/
//}


/*void macDrmCleanupLibKMS()
{
	//Ponemos todos los píxels a 0 
	memset(drmDevice->mappedmem,0,
	drmDevice->mode.hdisplay*drmDevice->mode.vdisplay);

	// restore saved CRTC configuration
	drmModeSetCrtc(drmDevice->fd,
		       drmDevice->saved_crtc->crtc_id,
		       drmDevice->saved_crtc->buffer_id,
		       drmDevice->saved_crtc->x,
		       drmDevice->saved_crtc->y,
		       &drmDevice->connector->connector_id,
		       1,
		       &drmDevice->saved_crtc->mode);
	drmModeFreeCrtc(drmDevice->saved_crtc);

	// unmap buffer
	munmap(drmDevice->mappedmem, drmDevice->size);

	// delete framebuffer
	drmModeRmFB(drmDevice->fd, drmDevice->fb);

	kms_destroy(&drmDevice->kms);	
	
	//liberar recursos
	drmModeFreeCrtc(drmDevice->saved_crtc);
	drmModeFreeCrtc(drmDevice->crtc);
	drmModeFreeEncoder(drmDevice->encoder);
	drmModeFreeConnector(drmDevice->connector);
	drmModeFreeResources(drmDevice->resources);

	//free allocated memory
	free(drmDevice);

	close (drmDevice->fd);
	exit(0);
}*/

void macDrmCleanup()
{
	//Ponemos todos los píxels a 0 
	memset(drmDevice->mappedmem,0,
	drmDevice->mode.hdisplay*drmDevice->mode.vdisplay);

	/* restore saved CRTC configuration */
	drmModeSetCrtc(drmDevice->fd,
		       drmDevice->saved_crtc->crtc_id,
		       drmDevice->saved_crtc->buffer_id,
		       drmDevice->saved_crtc->x,
		       drmDevice->saved_crtc->y,
		       &drmDevice->connector->connector_id,
		       1,
		       &drmDevice->saved_crtc->mode);
	drmModeFreeCrtc(drmDevice->saved_crtc);

	/* unmap buffer */
	munmap(drmDevice->mappedmem, drmDevice->size);

	/* delete framebuffer */
	drmModeRmFB(drmDevice->fd, drmDevice->fb);

	//Liberamos resources
	drmModeFreeCrtc(drmDevice->saved_crtc);
	drmModeFreeCrtc(drmDevice->crtc);
	drmModeFreeEncoder(drmDevice->encoder);
	drmModeFreeConnector(drmDevice->connector);
	drmModeFreeResources(drmDevice->resources);

	/* free allocated memory */
	free(drmDevice);

	close (drmDevice->fd);
	exit(0);
}

void macDrmPutPixel( int x, int y, uint8_t r, uint8_t g, uint8_t b) {
	//MAC Cuidado: esta implementación es para formato de píxel RGB x888, de 32 bits.
	unsigned int off;
	//Punto 1,1 en el extremo superior izquierdo de la pantalla
	//off = drmDevice->stride * y + x * 4;
	//Punto 1,1 en el extremo inferior izquierdo de la pantalla
	off = drmDevice->size - (y * drmDevice->stride ) + x * 4;
	*(uint32_t*)&drmDevice->mappedmem[off] = (r << 16) | (g << 8) | b;
}

/*static struct kms_bo *
drmAllocateBuffer(struct kms_driver *kms,
int width, int height, int *stride)
{
	struct kms_bo *bo;
	unsigned bo_attribs[] = {
		KMS_WIDTH, 0,
		KMS_HEIGHT, 0,
		KMS_BO_TYPE, KMS_BO_TYPE_SCANOUT_X8R8G8B8,
		KMS_TERMINATE_PROP_LIST
	};

	int ret;
	bo_attribs[1] = width;
	bo_attribs[3] = height;
	ret = kms_bo_create(kms, bo_attribs, &bo);

	if (ret) {
		printf("failed to alloc buffer: %s\n", strerror(-ret));
		return NULL;
	}

	ret = kms_bo_get_prop(bo, KMS_PITCH, stride);
	if (ret) {
		printf("failed to retreive buffer stride: %s\n", strerror(-ret));
		kms_bo_destroy(&bo);
		return NULL;
	}
	
	return bo;
}*/

void macPlane (){
	uint32_t width = 320;
	uint32_t height = 240;
	uint32_t src_width, src_height, src_offsetx, src_offsety;

	src_offsetx = (drmDevice->mode.hdisplay  - width)/2;
	src_offsety = (drmDevice->mode.vdisplay  - height)/2;
	//src_offsetx = 0;
	//src_offsety = 0;
	
	printf ("src_offsetx = %d, src_offsety = %d\n",
		src_offsetx, src_offsety);	
	
	src_width =  width << 16;
	src_height = height << 16;
	src_offsetx = src_offsetx << 16;
	src_offsety = src_offsety << 16;	

	int i;
	drmModePlaneRes *plane_resources;
	drmModePlane *ovr;	
	uint32_t plane_id;
	
	plane_resources = drmModeGetPlaneResources(drmDevice->fd);	
	if (!plane_resources)
                printf ("\nERROR - No se pudo recuperar info de overlays\n");

	printf ("MAC Disponemos de %d planos\n", plane_resources->count_planes);
	
	//Buscamos un plano que pueda usar el crtc que estamos usando
	for (i = 0; i < plane_resources->count_planes; i++) {
		ovr = drmModeGetPlane(drmDevice->fd, plane_resources->planes[i]);
                if (ovr->possible_crtcs & drmDevice->encoder->crtc_id){
                        plane_id = ovr->plane_id;
			printf ("MAC Usando plano(overlay) con Id %d\n", plane_id );
			
			/*printf("MAC formatos de pixel disponibles por el plano:");
			int j;
			for (j = 0; j < ovr->count_formats; j++)
				printf(" %4.4s", (char *)&ovr->formats[j]);
			printf("\n");*/

			break;
		}
		drmModeFreePlane(ovr);
        }
        
	if (!plane_id) {
                printf("\nERROR - No se pudo encontrar ningún overlay compatible con el CRTC en uso\n");
        }	

	//MAC: Cuidado, aquí tendrás que hacer cosas del doble buffer cuando lo implementes
	int ret = drmModeSetPlane(drmDevice->fd, plane_id, drmDevice->encoder->crtc_id, drmDevice->fb,
		0,0,0, drmDevice->mode.hdisplay, drmDevice->mode.vdisplay, src_offsetx, src_offsety, 
		src_width, src_height);
	if (ret)
		printf("ERROR - no se pudo establecer plano de overlay para escalado: %s\n", strerror(-ret));
	
}
